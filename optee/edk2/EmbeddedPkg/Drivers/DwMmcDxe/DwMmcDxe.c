/** @file
  This file implement the MMC Host Protocol for the DesignWare MMC.

  Copyright (c) 2014, Linaro Limited. All rights reserved.
  Copyright (c) 2014, Hisilicon Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Protocol/MmcHost.h>

#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#include "DwMmc.h"

#define DWMMC_DESC_PAGE		1
#define DWMMC_BLOCK_SIZE	512
#define DWMMC_DMA_BUF_SIZE	(512 * 8)

//#define EARLY_DUMP
//#define INIT_DUMP
//#define HACK_CMD8_DUMP

//#define EARLY_CMD8_DUMP
//#define DUMP_BUF

typedef struct {
  UINT32		Des0;
  UINT32		Des1;
  UINT32		Des2;
  UINT32		Des3;
} DWMMC_IDMAC_DESCRIPTOR;

EFI_MMC_HOST_PROTOCOL     *gpMmcHost;
EFI_GUID mDwMmcDevicePathGuid = EFI_CALLER_ID_GUID;
STATIC UINT32 mDwMmcCommand;
STATIC UINT32 mDwMmcArgument;

EFI_STATUS
DwMmcReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  );

BOOLEAN
DwMmcIsPowerOn (
  VOID
  )
{
    return TRUE;
}

EFI_STATUS
DwMmcInitialize (
  VOID
  )
{
    DEBUG ((EFI_D_BLKIO, "DwMmcInitialize()"));
    return EFI_SUCCESS;
}

BOOLEAN
DwMmcIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  return TRUE;
}

BOOLEAN
DwMmcIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  /* FIXME */
  return FALSE;
}

BOOLEAN
DwMmcIsDmaSupported (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  return TRUE;
}

EFI_STATUS
DwMmcBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL   **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL *NewDevicePathNode;

  NewDevicePathNode = CreateDeviceNode (HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (& ((VENDOR_DEVICE_PATH*)NewDevicePathNode)->Guid, &mDwMmcDevicePathGuid);

  *DevicePath = NewDevicePathNode;
  return EFI_SUCCESS;
}

EFI_STATUS
DwMmcUpdateClock (
  VOID
  )
{
  UINT32 Data;

  /* CMD_UPDATE_CLK */
  Data = BIT_CMD_WAIT_PRVDATA_COMPLETE | BIT_CMD_UPDATE_CLOCK_ONLY |
	 BIT_CMD_START;
  MmioWrite32 (DWMMC_CMD, Data);
  while (1) {
    Data = MmioRead32 (DWMMC_CMD);
    if (!(Data & CMD_START_BIT))
      break;
    Data = MmioRead32 (DWMMC_RINTSTS);
    if (Data & DWMMC_INT_HLE)
    {
      Print (L"failed to update mmc clock frequency\n");
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
DwMmcSetClock (
  IN UINTN                     ClockFreq
  )
{
  UINT32 Divider, Rate, Data;
  EFI_STATUS Status;
  BOOLEAN Found = FALSE;

  for (Divider = 1; Divider < 256; Divider++) {
    Rate = PcdGet32 (PcdDwMmcClockFrequencyInHz);
    if ((Rate / (2 * Divider)) <= ClockFreq) {
      Found = TRUE;
      break;
    }
  }
  if (Found == FALSE)
    return EFI_NOT_FOUND;

  // Wait until MMC is idle
  do {
    Data = MmioRead32 (DWMMC_STATUS);
  } while (Data & DWMMC_STS_DATA_BUSY);

  // Disable MMC clock first
  MmioWrite32 (DWMMC_CLKENA, 0);
  Status = DwMmcUpdateClock ();
  ASSERT (!EFI_ERROR (Status));

  MmioWrite32 (DWMMC_CLKDIV, Divider);
  Status = DwMmcUpdateClock ();
  ASSERT (!EFI_ERROR (Status));

  // Enable MMC clock
  MmioWrite32 (DWMMC_CLKENA, 1);
  MmioWrite32 (DWMMC_CLKSRC, 0);
  Status = DwMmcUpdateClock ();
  ASSERT (!EFI_ERROR (Status));
  return EFI_SUCCESS;
}

EFI_STATUS
DwMmcNotifyState (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_STATE                 State
  )
{
  UINT32      Data;
  EFI_STATUS  Status;
#ifdef INIT_DUMP
  VOID*         Buffer;
#endif

  switch (State) {
  case MmcInvalidState:
    ASSERT (0);
    break;
  case MmcHwInitializationState:
    MmioWrite32 (DWMMC_PWREN, 1);

    // If device already turn on then restart it
    Data = DWMMC_CTRL_RESET_ALL;
    MmioWrite32 (DWMMC_CTRL, Data);
    do {
      // Wait until reset operation finished
      Data = MmioRead32 (DWMMC_CTRL);
    } while (Data & DWMMC_CTRL_RESET_ALL);

    // Setup clock that could not be higher than 400KHz.
    Status = DwMmcSetClock (400000);
    ASSERT (!EFI_ERROR (Status));
    MicroSecondDelay (100);

    MmioWrite32 (DWMMC_RINTSTS, ~0);
    MmioWrite32 (DWMMC_INTMASK, 0);
    MmioWrite32 (DWMMC_TMOUT, ~0);
    MmioWrite32 (DWMMC_IDINTEN, 0);
    MmioWrite32 (DWMMC_BMOD, DWMMC_IDMAC_SWRESET);

    MmioWrite32 (DWMMC_BLKSIZ, DWMMC_BLOCK_SIZE);
    do {
      Data = MmioRead32 (DWMMC_BMOD);
    } while (Data & DWMMC_IDMAC_SWRESET);


#if 0
    Data = DWMMC_DMA_BURST_SIZE(2) | DWMMC_FIFO_TWMARK(8) | DWMMC_FIFO_RWMARK(7);
    MmioWrite32 (DWMMC_FIFOTH, Data);
    Data = DWMMC_CARD_RD_THR(512) | DWMMC_CARD_RD_THR_EN;
    MmioWrite32 (DWMMC_CARDTHRCTL, Data);
#endif

    // Set Data Length & Data Timer
//    MmioWrite32 (DWMMC_CTYPE, MMC_8BIT_MODE);
//    MmioWrite32 (DWMMC_DEBNCE, 0x00ffffff);

#ifdef INIT_DUMP
  Buffer = UncachedAllocatePages (2);
  if (Buffer == NULL)
    return EFI_BUFFER_TOO_SMALL;
  DwMmcReadBlockData (NULL, 0, 512, Buffer);
#endif
    break;
  case MmcIdleState:
    break;
  case MmcReadyState:
    break;
  case MmcIdentificationState:
    break;
  case MmcStandByState:
    break;
  case MmcTransferState:
    break;
  case MmcSendingDataState:
    break;
  case MmcReceiveDataState:
    break;
  case MmcProgrammingState:
    break;
  case MmcDisconnectState:
    break;
  default:
    ASSERT (0);
  }
  return EFI_SUCCESS;
}

// Need to prepare DMA buffer first before sending commands to MMC card
BOOLEAN
IsPendingReadCommand (
  IN MMC_CMD                    MmcCmd
  )
{
  UINTN  Mask;

  Mask = BIT_CMD_DATA_EXPECTED | BIT_CMD_READ;
  if ((MmcCmd & Mask) == Mask)
    return TRUE;
  return FALSE;
}

BOOLEAN
IsPendingWriteCommand (
  IN MMC_CMD                    MmcCmd
  )
{
  UINTN  Mask;

  Mask = BIT_CMD_DATA_EXPECTED | BIT_CMD_WRITE;
  if ((MmcCmd & Mask) == Mask)
    return TRUE;
  return FALSE;
}

EFI_STATUS
SendCommand (
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  )
{
  UINT32      Data, ErrMask;

  // Wait until MMC is idle
  do {
    Data = MmioRead32 (DWMMC_STATUS);
  } while (Data & DWMMC_STS_DATA_BUSY);

  MmioWrite32 (DWMMC_RINTSTS, ~0);
  MmioWrite32 (DWMMC_CMDARG, Argument);
  MmioWrite32 (DWMMC_CMD, MmcCmd);

  ErrMask = DWMMC_INT_EBE | DWMMC_INT_HLE | DWMMC_INT_RTO |
            DWMMC_INT_RCRC | DWMMC_INT_RE;
  ErrMask |= DWMMC_INT_DCRC | DWMMC_INT_DRT | DWMMC_INT_SBE;
  do {
    MicroSecondDelay(500);
    Data = MmioRead32 (DWMMC_RINTSTS);

    if (Data & ErrMask)
      return EFI_DEVICE_ERROR;
    if (Data & DWMMC_INT_DTO)	// Transfer Done
      break;
  } while (!(Data & DWMMC_INT_CMD_DONE));
  return EFI_SUCCESS;
}

EFI_STATUS
DwMmcSendCommand (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  )
{
  UINT32       Cmd = 0;
  EFI_STATUS   Status = EFI_SUCCESS;

  switch (MMC_GET_INDX(MmcCmd)) {
  case MMC_INDX(0):
    Cmd = BIT_CMD_SEND_INIT;
    break;
  case MMC_INDX(1):
    Cmd = BIT_CMD_RESPONSE_EXPECT;
    break;
  case MMC_INDX(2):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_LONG_RESPONSE |
           BIT_CMD_CHECK_RESPONSE_CRC | BIT_CMD_SEND_INIT;
    break;
  case MMC_INDX(3):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_SEND_INIT;
    break;
  case MMC_INDX(7):
    if (Argument)
        Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    else
        Cmd = 0;
    break;
  case MMC_INDX(8):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(9):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_LONG_RESPONSE;
    break;
  case MMC_INDX(12):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_STOP_ABORT_CMD;
    break;
  case MMC_INDX(13):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(17):
  case MMC_INDX(18):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(24):
  case MMC_INDX(25):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_WRITE |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(30):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED;
    break;
  default:
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    break;
  }

  Cmd |= MMC_GET_INDX(MmcCmd) | BIT_CMD_USE_HOLD_REG | BIT_CMD_START;
  if (IsPendingReadCommand (Cmd) || IsPendingWriteCommand (Cmd)) {
    mDwMmcCommand = Cmd;
    mDwMmcArgument = Argument;
  } else {
    Status = SendCommand (Cmd, Argument);
  }
  return Status;
}

EFI_STATUS
DwMmcReceiveResponse (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_RESPONSE_TYPE          Type,
  IN UINT32*                    Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (   (Type == MMC_RESPONSE_TYPE_R1)
      || (Type == MMC_RESPONSE_TYPE_R1b)
      || (Type == MMC_RESPONSE_TYPE_R3)
      || (Type == MMC_RESPONSE_TYPE_R6)
      || (Type == MMC_RESPONSE_TYPE_R7))
  {
    Buffer[0] = MmioRead32 (DWMMC_RESP0);
  } else if (Type == MMC_RESPONSE_TYPE_R2) {
    Buffer[0] = MmioRead32 (DWMMC_RESP0);
    Buffer[1] = MmioRead32 (DWMMC_RESP1);
    Buffer[2] = MmioRead32 (DWMMC_RESP2);
    Buffer[3] = MmioRead32 (DWMMC_RESP3);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
PrepareDmaData (
  IN DWMMC_IDMAC_DESCRIPTOR*    IdmacDesc,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  UINTN  Cnt, Blks, Idx, LastIdx;
  UINT32 Data;

  Cnt = (Length + DWMMC_DMA_BUF_SIZE - 1) / DWMMC_DMA_BUF_SIZE;
  Blks = (Length + DWMMC_BLOCK_SIZE - 1) / DWMMC_BLOCK_SIZE;
  Length = DWMMC_BLOCK_SIZE * Blks;

  for (Idx = 0; Idx < Cnt; Idx++) {
    (IdmacDesc + Idx)->Des0 = DWMMC_IDMAC_DES0_OWN | DWMMC_IDMAC_DES0_CH |
	    		      DWMMC_IDMAC_DES0_DIC;
    (IdmacDesc + Idx)->Des1 = DWMMC_IDMAC_DES1_BS1(DWMMC_DMA_BUF_SIZE);
    /* Buffer Address */
    (IdmacDesc + Idx)->Des2 = (UINT32)((UINTN)Buffer + DWMMC_DMA_BUF_SIZE * Idx);
    /* Next Descriptor Address */
    (IdmacDesc + Idx)->Des3 = (UINT32)((UINTN)IdmacDesc +
   	                               (sizeof(DWMMC_IDMAC_DESCRIPTOR) * (Idx + 1)));
  }
  /* First Descriptor */
  IdmacDesc->Des0 |= DWMMC_IDMAC_DES0_FS;
  /* Last Descriptor */
  LastIdx = Cnt - 1;
  (IdmacDesc + LastIdx)->Des0 |= DWMMC_IDMAC_DES0_LD;
  (IdmacDesc + LastIdx)->Des0 &= ~(DWMMC_IDMAC_DES0_DIC | DWMMC_IDMAC_DES0_CH);
  (IdmacDesc + LastIdx)->Des1 = DWMMC_IDMAC_DES1_BS1(Length -
   		                (LastIdx * DWMMC_DMA_BUF_SIZE));
  /* Set the Next field of Last Descriptor */
  (IdmacDesc + LastIdx)->Des3 = 0;
  MmioWrite32 (DWMMC_DBADDR, (UINT32)((UINTN)IdmacDesc));

  Data = MmioRead32 (DWMMC_CTRL);
  Data |= DWMMC_CTRL_INT_EN | DWMMC_CTRL_DMA_EN | DWMMC_CTRL_IDMAC_EN;
  MmioWrite32 (DWMMC_CTRL, Data);
  Data = MmioRead32 (DWMMC_BMOD);
  Data |= DWMMC_IDMAC_ENABLE | DWMMC_IDMAC_FB;
  MmioWrite32 (DWMMC_BMOD, Data);

  MmioWrite32 (DWMMC_BLKSIZ, DWMMC_BLOCK_SIZE);
  MmioWrite32 (DWMMC_BYTCNT, Length);

  return EFI_SUCCESS;
}

EFI_STATUS
DwMmcReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                   Buffer
  )
{
  DWMMC_IDMAC_DESCRIPTOR*  IdmacDesc;
  EFI_STATUS  Status;
  UINT32      DescPages, CountPerPage, Count;
#ifdef DUMP_BUF
  CHAR8       CBuffer[100];
  UINTN       CharCount, Idx;
#endif
  EFI_TPL     Tpl;

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  CountPerPage = EFI_PAGE_SIZE / 16;
  Count = (Length + DWMMC_DMA_BUF_SIZE - 1) / DWMMC_DMA_BUF_SIZE;
  DescPages = (Count + CountPerPage - 1) / CountPerPage;

  IdmacDesc = (DWMMC_IDMAC_DESCRIPTOR *)UncachedAllocatePages (DescPages);
  if (IdmacDesc == NULL) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto out;
  }

  InvalidateDataCacheRange (Buffer, Length);

  Status = PrepareDmaData (IdmacDesc, Length, Buffer);
  if (EFI_ERROR (Status))
    goto out_dma;

#if defined(EARLY_DUMP) || defined(INIT_DUMP) || defined(HACK_CMD8_DUMP)
  Status = SendCommand (17 | BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
		  BIT_CMD_DATA_EXPECTED | BIT_CMD_READ | BIT_CMD_WAIT_PRVDATA_COMPLETE |
		  BIT_CMD_USE_HOLD_REG | BIT_CMD_START, Lba);
#else
  Status = SendCommand (mDwMmcCommand, mDwMmcArgument);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to read data, mDwMmcCommand:%x, mDwMmcArgument:%x, Status:%r\n", mDwMmcCommand, mDwMmcArgument, Status));
    goto out_dma;
  }
#endif
#ifdef DUMP_BUF
  for (Idx = 0; Idx < Length; Idx += 8) {
    CharCount = AsciiSPrint (CBuffer,sizeof (CBuffer),"#%4x: %x %x %x %x %x %x %x %x\n", Idx,
	    *((UINT8 *)Buffer + Idx), *((UINT8 *)Buffer + Idx + 1), *((UINT8 *)Buffer + Idx + 2),
	    *((UINT8 *)Buffer + Idx + 3), *((UINT8 *)Buffer + Idx + 4), *((UINT8 *)Buffer + Idx + 5),
	    *((UINT8 *)Buffer + Idx + 6), *((UINT8 *)Buffer + Idx + 7));
    SerialPortWrite ((UINT8 *) CBuffer, CharCount);
  }
#endif
out_dma:
  UncachedFreePages (IdmacDesc, DescPages);
out:
  // Restore Tpl
  gBS->RestoreTPL (Tpl);
  return Status;
}

EFI_STATUS
DwMmcWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  DWMMC_IDMAC_DESCRIPTOR*  IdmacDesc;
  EFI_STATUS  Status;
  UINT32      DescPages, CountPerPage, Count;
  EFI_TPL     Tpl;

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  CountPerPage = EFI_PAGE_SIZE / 16;
  Count = (Length + DWMMC_DMA_BUF_SIZE - 1) / DWMMC_DMA_BUF_SIZE;
  DescPages = (Count + CountPerPage - 1) / CountPerPage;
  IdmacDesc = (DWMMC_IDMAC_DESCRIPTOR *)UncachedAllocatePages (DescPages);
  if (IdmacDesc == NULL) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto out;
  }

  WriteBackDataCacheRange (Buffer, Length);

  Status = PrepareDmaData (IdmacDesc, Length, Buffer);
  if (EFI_ERROR (Status))
    goto out_dma;

  Status = SendCommand (mDwMmcCommand, mDwMmcArgument);
out_dma:
  UncachedFreePages (IdmacDesc, DescPages);
out:
  // Restore Tpl
  gBS->RestoreTPL (Tpl);
  return Status;
}

EFI_STATUS
DwMmcSetIos (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN  UINT32                    BusClockFreq,
  IN  UINT32                    BusWidth,
  IN  UINT32                    TimingMode
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32    Data;

  if (TimingMode != EMMCBACKWARD) {
    Data = MmioRead32 (DWMMC_UHSREG);
    switch (TimingMode) {
    case EMMCHS52DDR1V2:
    case EMMCHS52DDR1V8:
      Data |= 1 << 16;
      break;
    case EMMCHS52:
    case EMMCHS26:
      Data &= ~(1 << 16);
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    MmioWrite32 (DWMMC_UHSREG, Data);
  }

  switch (BusWidth) {
  case 1:
    MmioWrite32 (DWMMC_CTYPE, 0);
    break;
  case 4:
    MmioWrite32 (DWMMC_CTYPE, 1);
    break;
  case 8:
    MmioWrite32 (DWMMC_CTYPE, 1 << 16);
    break;
  default:
    return EFI_UNSUPPORTED;
  }
  if (BusClockFreq) {
    Status = DwMmcSetClock (BusClockFreq);
  }
  return Status;
}

EFI_MMC_HOST_PROTOCOL gMciHost = {
  MMC_HOST_PROTOCOL_REVISION,
  DwMmcIsCardPresent,
  DwMmcIsReadOnly,
  DwMmcIsDmaSupported,
  DwMmcBuildDevicePath,
  DwMmcNotifyState,
  DwMmcSendCommand,
  DwMmcReceiveResponse,
  DwMmcReadBlockData,
  DwMmcWriteBlockData,
  DwMmcSetIos
};

EFI_STATUS
DwMmcDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;
#if defined(EARLY_DUMP) || defined(EARLY_CMD8_DUMP)
  VOID*         Buffer;
#endif

  Handle = NULL;

#if defined(EARLY_DUMP) || defined(EARLY_CMD8_DUMP)
  Buffer = UncachedAllocatePages (2);
  if (Buffer == NULL)
    return EFI_BUFFER_TOO_SMALL;
#endif
#ifdef EARLY_DUMP
  DwMmcReadBlockData (NULL, 0, 512, Buffer);
#endif
#ifdef EARLY_CMD8_DUMP
  DwMmcSendCommand (NULL, 8, 1 << 16);
  DwMmcReadBlockData (NULL, 0, 512, Buffer);
#endif
  DEBUG ((EFI_D_BLKIO, "DwMmcDxeInitialize()\n"));

  //Publish Component Name, BlockIO protocol interfaces
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiMmcHostProtocolGuid,         &gMciHost,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
