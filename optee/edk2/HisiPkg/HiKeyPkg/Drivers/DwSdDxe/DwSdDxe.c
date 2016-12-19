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

#include <Library/ArmLib.h>
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

#include "DwSd.h"

#define DWSD_DESC_PAGE		1
#define DWSD_BLOCK_SIZE	        512
#define DWSD_DMA_BUF_SIZE	(512 * 8)

#define DWSD_DMA_THRESHOLD	16

//#define FIFO
//#define DUMP_BUF

typedef struct {
  UINT32		Des0;
  UINT32		Des1;
  UINT32		Des2;
  UINT32		Des3;
} DWSD_IDMAC_DESCRIPTOR;

EFI_MMC_HOST_PROTOCOL     *gpMmcHost;
EFI_GUID mDwSdDevicePathGuid = EFI_CALLER_ID_GUID;
STATIC UINT32 mDwSdCommand;
STATIC UINT32 mDwSdArgument;

EFI_STATUS
DwSdSendCommand (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  );
EFI_STATUS
DwSdReceiveResponse (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_RESPONSE_TYPE          Type,
  IN UINT32*                    Buffer
  );

EFI_STATUS
DwSdReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  );

BOOLEAN
DwSdIsPowerOn (
  VOID
  )
{
    return TRUE;
}

EFI_STATUS
DwSdInitialize (
  VOID
  )
{
    DEBUG ((EFI_D_BLKIO, "DwSdInitialize()"));
    return EFI_SUCCESS;
}

BOOLEAN
DwSdIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  UINT32    Value;

  /*
   * FIXME
   * At first, reading GPIO pin shouldn't exist in SD driver. We need to
   * add some callbacks to handle settings for hardware platform.
   * In the second, reading GPIO pin should be based on GPIO driver. Now
   * GPIO driver could only be used for one PL061 gpio controller. And it's
   * used to detect jumper setting. As a workaround, we have to read the gpio
   * register instead at here.
   *
   */
  Value = MmioRead32 (0xf8012000 + (1 << 2));
  if (Value)
    return FALSE;
  return TRUE;
}

BOOLEAN
DwSdIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  /* FIXME */
  return FALSE;
}

BOOLEAN
DwSdIsDmaSupported (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
#ifdef FIFO
  return FALSE;
#else
  return TRUE;
#endif
}

EFI_STATUS
DwSdBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL   **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL *NewDevicePathNode;

  NewDevicePathNode = CreateDeviceNode (HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (& ((VENDOR_DEVICE_PATH*)NewDevicePathNode)->Guid, &mDwSdDevicePathGuid);

  *DevicePath = NewDevicePathNode;
  return EFI_SUCCESS;
}

EFI_STATUS
DwSdUpdateClock (
  VOID
  )
{
  UINT32 Data;

  /* CMD_UPDATE_CLK */
  Data = BIT_CMD_WAIT_PRVDATA_COMPLETE | BIT_CMD_UPDATE_CLOCK_ONLY |
	 BIT_CMD_START;
  MmioWrite32 (DWSD_CMD, Data);
  while (1) {
    Data = MmioRead32 (DWSD_CMD);
    if (!(Data & CMD_START_BIT))
      break;
    Data = MmioRead32 (DWSD_RINTSTS);
    if (Data & DWSD_INT_HLE)
    {
      Print (L"failed to update mmc clock frequency\n");
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
DwSdSetClock (
  IN UINTN                     ClockFreq
  )
{
  UINT32 Divider, Rate, Data;
  EFI_STATUS Status;
  BOOLEAN Found = FALSE;

  for (Divider = 1; Divider < 256; Divider++) {
    Rate = PcdGet32 (PcdDwSdClockFrequencyInHz);
    if ((Rate / (2 * Divider)) <= ClockFreq) {
      Found = TRUE;
      break;
    }
  }
  if (Found == FALSE)
    return EFI_NOT_FOUND;

  // Wait until MMC is idle
  do {
    Data = MmioRead32 (DWSD_STATUS);
  } while (Data & DWSD_STS_DATA_BUSY);

  // Disable MMC clock first
  MmioWrite32 (DWSD_CLKENA, 0);
  Status = DwSdUpdateClock ();
  ASSERT (!EFI_ERROR (Status));

  MmioWrite32 (DWSD_CLKDIV, Divider);
  Status = DwSdUpdateClock ();
  ASSERT (!EFI_ERROR (Status));

  // Enable MMC clock
  MmioWrite32 (DWSD_CLKENA, 1);
  MmioWrite32 (DWSD_CLKSRC, 0);
  Status = DwSdUpdateClock ();
  ASSERT (!EFI_ERROR (Status));
  return EFI_SUCCESS;
}

EFI_STATUS
DwSdNotifyState (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_STATE                 State
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  switch (State) {
  case MmcInvalidState:
    ASSERT (0);
    break;
  case MmcHwInitializationState:
    MmioWrite32 (DWSD_PWREN, 1);

    // If device already turn on then restart it
    Data = DWSD_CTRL_RESET_ALL;
    MmioWrite32 (DWSD_CTRL, Data);
    do {
      // Wait until reset operation finished
      Data = MmioRead32 (DWSD_CTRL);
    } while (Data & DWSD_CTRL_RESET_ALL);

    MmioWrite32 (DWSD_RINTSTS, ~0);
    MmioWrite32 (DWSD_INTMASK, 0);
    MmioWrite32 (DWSD_TMOUT, ~0);
    MmioWrite32 (DWSD_IDINTEN, 0);
    MmioWrite32 (DWSD_BMOD, DWSD_IDMAC_SWRESET);

    MmioWrite32 (DWSD_BLKSIZ, DWSD_BLOCK_SIZE);
    do {
      Data = MmioRead32 (DWSD_BMOD);
    } while (Data & DWSD_IDMAC_SWRESET);


    Data = DWSD_DMA_BURST_SIZE(2) | DWSD_FIFO_TWMARK(8) | DWSD_FIFO_RWMARK(7) | (2 << 28);
    MmioWrite32 (DWSD_FIFOTH, Data);
    Data = DWSD_CARD_RD_THR(512) | DWSD_CARD_RD_THR_EN;
    MmioWrite32 (DWSD_CARDTHRCTL, Data);

    // Set Data Length & Data Timer
    MmioWrite32 (DWSD_CTYPE, 0);
    MmioWrite32 (DWSD_DEBNCE, 0x00ffffff);

    // Setup clock that could not be higher than 400KHz.
    Status = DwSdSetClock (400000);
    ASSERT (!EFI_ERROR (Status));
    MicroSecondDelay (100);

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

EFI_STATUS
SendCommand (
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  )
{
  UINT32      Data, ErrMask;

  MmioWrite32 (DWSD_RINTSTS, ~0);
  MmioWrite32 (DWSD_CMDARG, Argument);
  MicroSecondDelay(500);
  // Wait until MMC is idle
  do {
    Data = MmioRead32 (DWSD_STATUS);
  } while (Data & DWSD_STS_DATA_BUSY);

  MmioWrite32 (DWSD_CMD, MmcCmd);

  ErrMask = DWSD_INT_EBE | DWSD_INT_HLE | DWSD_INT_RTO |
            DWSD_INT_RCRC | DWSD_INT_RE;
  ErrMask |= DWSD_INT_DCRC | DWSD_INT_DRT | DWSD_INT_SBE;
  do {
    MicroSecondDelay(500);
    Data = MmioRead32 (DWSD_RINTSTS);

    if (Data & ErrMask) {
      DEBUG ((EFI_D_ERROR, "Data:%x, ErrMask:%x, TBBCNT:%x, TCBCNT:%x, BYTCNT:%x, BLKSIZ:%x\n", Data, ErrMask, MmioRead32 (DWSD_TBBCNT), MmioRead32 (DWSD_TCBCNT), MmioRead32 (DWSD_BYTCNT), MmioRead32 (DWSD_BLKSIZ)));
      return EFI_DEVICE_ERROR;
    }
    if (Data & DWSD_INT_DTO)	// Transfer Done
      break;
  } while (!(Data & DWSD_INT_CMD_DONE));
  MmcCmd &= 0x3f;
  if (MmcCmd == 17)
    MicroSecondDelay(100);
  else if (MmcCmd != 13)
    MicroSecondDelay(5000);

  return EFI_SUCCESS;
}

UINTN ACmd = 0;

EFI_STATUS
DwSdSendCommand (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  )
{
  UINT32       Cmd = 0;
  EFI_STATUS   Status = EFI_SUCCESS;
  BOOLEAN      Pending = FALSE;
  UINT32       Data;

  switch (MMC_GET_INDX(MmcCmd)) {
  case MMC_INDX(0):
    //Cmd = BIT_CMD_SEND_INIT;
    Cmd = BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(1):
    Cmd = BIT_CMD_RESPONSE_EXPECT;
    break;
  case MMC_INDX(2):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_LONG_RESPONSE |
           BIT_CMD_CHECK_RESPONSE_CRC | BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(3):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(6):
    if (!ACmd) {
      Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
	    BIT_CMD_DATA_EXPECTED | BIT_CMD_WAIT_PRVDATA_COMPLETE;
    } else {
      Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
	    BIT_CMD_WAIT_PRVDATA_COMPLETE;
    }
#ifdef FIFO
    Pending = FALSE;
    Data = MmioRead32 (DWSD_CTRL);
    Data |= DWSD_CTRL_FIFO_RESET;
    MmioWrite32 (DWSD_CTRL, Data);
    while (MmioRead32 (DWSD_CTRL) & DWSD_CTRL_FIFO_RESET) {
    };
#else
    if (!ACmd)
      Pending = TRUE;
#endif
    break;
  case MMC_INDX(7):
    if (Argument)
        Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
	       BIT_CMD_WAIT_PRVDATA_COMPLETE;
    else
        Cmd = 0;
    break;
  case MMC_INDX(8):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           //BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
           BIT_CMD_READ |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(9):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_LONG_RESPONSE | BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(12):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_STOP_ABORT_CMD;
    break;
  case MMC_INDX(13):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    break;
  case MMC_INDX(17):
  case MMC_INDX(18):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
#ifdef FIFO
    Pending = FALSE;
    Data = MmioRead32 (DWSD_CTRL);
    Data |= DWSD_CTRL_FIFO_RESET;
    MmioWrite32 (DWSD_CTRL, Data);
    while (MmioRead32 (DWSD_CTRL) & DWSD_CTRL_FIFO_RESET) {
    };
#else
    Pending = TRUE;
    Data = MmioRead32 (DWSD_CTRL);
    Data |= DWSD_CTRL_FIFO_RESET;
    MmioWrite32 (DWSD_CTRL, Data);
    while (MmioRead32 (DWSD_CTRL) & DWSD_CTRL_FIFO_RESET) {
    };
#endif
    break;
  case MMC_INDX(24):
  case MMC_INDX(25):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_WRITE |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
#ifdef FIFO
    Pending = FALSE;
    Data = MmioRead32 (DWSD_CTRL);
    Data |= DWSD_CTRL_FIFO_RESET;
    MmioWrite32 (DWSD_CTRL, Data);
    while (MmioRead32 (DWSD_CTRL) & DWSD_CTRL_FIFO_RESET) {
    };
#else
    Pending = TRUE;
#endif
    break;
  case MMC_INDX(30):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED;
    break;
  case MMC_INDX(41):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(51):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
#ifdef FIFO
    Pending = FALSE;
    Data = MmioRead32 (DWSD_CTRL);
    Data |= DWSD_CTRL_FIFO_RESET;
    MmioWrite32 (DWSD_CTRL, Data);
    while (MmioRead32 (DWSD_CTRL) & DWSD_CTRL_FIFO_RESET) {
    };
#else
    Pending = TRUE;
#endif
    break;
  case MMC_INDX(55):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    ACmd = 1;
    break;
  default:
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    break;
  }

  Cmd |= MMC_GET_INDX(MmcCmd) | BIT_CMD_USE_HOLD_REG | BIT_CMD_START;
  if (Pending) {
    mDwSdCommand = Cmd;
    mDwSdArgument = Argument;
  } else {
    mDwSdCommand = 0;
    mDwSdArgument = 0;
    Status = SendCommand (Cmd, Argument);
  }
  /* Clear ACMD */
  if (MMC_GET_INDX(MmcCmd) != MMC_INDX(55))
    ACmd = 0;
  return Status;
}

EFI_STATUS
DwSdReceiveResponse (
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
    Buffer[0] = MmioRead32 (DWSD_RESP0);
  } else if (Type == MMC_RESPONSE_TYPE_R2) {
    Buffer[0] = MmioRead32 (DWSD_RESP0);
    Buffer[1] = MmioRead32 (DWSD_RESP1);
    Buffer[2] = MmioRead32 (DWSD_RESP2);
    Buffer[3] = MmioRead32 (DWSD_RESP3);
  }
  return EFI_SUCCESS;
}

#ifndef FIFO
EFI_STATUS
PrepareDmaData (
  IN DWSD_IDMAC_DESCRIPTOR*    IdmacDesc,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  UINTN  Cnt, Idx, LastIdx, BlockSize;
  UINT32 Data;

  if (Length % 4) {
    DEBUG ((EFI_D_ERROR, "Length isn't aligned with 4\n"));
    return EFI_BAD_BUFFER_SIZE;
  }
  if (Length < DWSD_DMA_THRESHOLD) {
    return EFI_BUFFER_TOO_SMALL;
  }
  Cnt = (Length + DWSD_DMA_BUF_SIZE - 1) / DWSD_DMA_BUF_SIZE;
  if (Length > DWSD_BLOCK_SIZE)
    BlockSize = DWSD_BLOCK_SIZE;
  else {
    BlockSize = Length;
  }

  for (Idx = 0; Idx < Cnt; Idx++) {
    (IdmacDesc + Idx)->Des0 = DWSD_IDMAC_DES0_OWN | DWSD_IDMAC_DES0_CH |
	    		      DWSD_IDMAC_DES0_DIC;
    (IdmacDesc + Idx)->Des1 = DWSD_IDMAC_DES1_BS1(DWSD_DMA_BUF_SIZE);
    /* Buffer Address */
    (IdmacDesc + Idx)->Des2 = (UINT32)((UINTN)Buffer + DWSD_DMA_BUF_SIZE * Idx);
    /* Next Descriptor Address */
    (IdmacDesc + Idx)->Des3 = (UINT32)((UINTN)IdmacDesc +
   	                               (sizeof(DWSD_IDMAC_DESCRIPTOR) * (Idx + 1)));
  }
  /* First Descriptor */
  IdmacDesc->Des0 |= DWSD_IDMAC_DES0_FS;
  /* Last Descriptor */
  LastIdx = Cnt - 1;
  (IdmacDesc + LastIdx)->Des0 |= DWSD_IDMAC_DES0_LD;
  (IdmacDesc + LastIdx)->Des0 &= ~(DWSD_IDMAC_DES0_DIC | DWSD_IDMAC_DES0_CH);
  (IdmacDesc + LastIdx)->Des1 = DWSD_IDMAC_DES1_BS1(Length -
   		                (LastIdx * DWSD_DMA_BUF_SIZE));
  /* Set the Next field of Last Descriptor */
  (IdmacDesc + LastIdx)->Des3 = 0;
  MmioWrite32 (DWSD_DBADDR, (UINT32)((UINTN)IdmacDesc));

  Data = MmioRead32 (DWSD_CTRL);
  Data |= DWSD_CTRL_INT_EN | DWSD_CTRL_DMA_EN | DWSD_CTRL_IDMAC_EN;
  MmioWrite32 (DWSD_CTRL, Data);
  Data = MmioRead32 (DWSD_BMOD);
  Data |= DWSD_IDMAC_ENABLE | DWSD_IDMAC_FB;
  MmioWrite32 (DWSD_BMOD, Data);

  MmioWrite32 (DWSD_BLKSIZ, BlockSize);
  MmioWrite32 (DWSD_BYTCNT, Length);

  return EFI_SUCCESS;
}
#endif

STATIC
EFI_STATUS
ReadFifo (
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  UINT32      Data, Received, Count;
#ifdef DUMP_BUF
  CHAR8       CBuffer[100];
  UINTN       CharCount, Idx;
#endif

  Received = 0;
  Count = (Length + 3) / 4;
  while (Received < Count) {
    Data = MmioRead32 (DWSD_RINTSTS);
    if (Data & DWSD_INT_CMD_DONE) {
      *(Buffer + Received) = MmioRead32 (DWSD_FIFO_START);
      Received++;
    } else {
      DEBUG ((EFI_D_ERROR, "Received:%d, RINTSTS:%x\n", Received, Data));
    }
  }
  while (1) {
    Data = MmioRead32 (DWSD_RINTSTS);
    if (Data & DWSD_INT_DTO)
      break;
  }
#ifdef DUMP_BUF
  for (Idx = 0; Idx < Length; Idx += 8) {
    CharCount = AsciiSPrint (CBuffer,sizeof (CBuffer),"#%4x: %x %x %x %x %x %x %x %x\n", Idx,
	    *((UINT8 *)Buffer + Idx), *((UINT8 *)Buffer + Idx + 1), *((UINT8 *)Buffer + Idx + 2),
	    *((UINT8 *)Buffer + Idx + 3), *((UINT8 *)Buffer + Idx + 4), *((UINT8 *)Buffer + Idx + 5),
	    *((UINT8 *)Buffer + Idx + 6), *((UINT8 *)Buffer + Idx + 7));
    SerialPortWrite ((UINT8 *) CBuffer, CharCount);
  }
  DEBUG ((EFI_D_ERROR, "TBB:%x, TCB:%x, BYTCNT:%x, BLKSIZ:%x\n", MmioRead32 (DWSD_TBBCNT), MmioRead32 (DWSD_TCBCNT), MmioRead32 (DWSD_BYTCNT), MmioRead32 (DWSD_BLKSIZ)));
#else
  /* FIXME */
  MicroSecondDelay (1000);
#endif
  return EFI_SUCCESS;
}

#ifdef FIFO
EFI_STATUS
DwSdReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  return ReadFifo (Length, Buffer);
}
#else
EFI_STATUS
DwSdReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                   Buffer
  )
{
  DWSD_IDMAC_DESCRIPTOR*  IdmacDesc;
  EFI_STATUS  Status;
  UINT32      DescPages, CountPerPage, Count, Data;
#ifdef DUMP_BUF
  CHAR8       CBuffer[100];
  UINTN       CharCount, Idx;
#endif

  CountPerPage = EFI_PAGE_SIZE / 16;
  Count = (Length + DWSD_DMA_BUF_SIZE - 1) / DWSD_DMA_BUF_SIZE;
  DescPages = (Count + CountPerPage - 1) / CountPerPage;

  IdmacDesc = (DWSD_IDMAC_DESCRIPTOR *)UncachedAllocatePages (DescPages);
  if (IdmacDesc == NULL)
    return EFI_BUFFER_TOO_SMALL;

  InvalidateDataCacheRange (Buffer, Length);

  Status = PrepareDmaData (IdmacDesc, Length, Buffer);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Data = MmioRead32 (DWSD_CTRL);
      Data |= DWSD_CTRL_FIFO_RESET;
      MmioWrite32 (DWSD_CTRL, Data);
      while (MmioRead32 (DWSD_CTRL) & DWSD_CTRL_FIFO_RESET) {
      };

      Status = SendCommand (mDwSdCommand, mDwSdArgument);
      if (EFI_ERROR (Status)) {
	DEBUG ((EFI_D_ERROR, "Failed to read data from FIFO, mDwSdCommand:%x, mDwSdArgument:%x, Status:%r\n", mDwSdCommand, mDwSdArgument, Status));
	goto out;
      }
      Status = ReadFifo (Length, Buffer);
    }
    goto done;
  } else {

    Status = SendCommand (mDwSdCommand, mDwSdArgument);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Failed to read data, mDwSdCommand:%x, mDwSdArgument:%x, Status:%r\n", mDwSdCommand, mDwSdArgument, Status));
      goto out;
    }

    /* Wait until data transfer finished */
    while (MmioRead32 (DWSD_TCBCNT) < MmioRead32 (DWSD_BYTCNT)) {
    }

  }
done:
#ifdef DUMP_BUF
  for (Idx = 0; Idx < Length; Idx += 8) {
    CharCount = AsciiSPrint (CBuffer,sizeof (CBuffer),"#%4x: %x %x %x %x %x %x %x %x\n", Idx,
	    *((UINT8 *)Buffer + Idx), *((UINT8 *)Buffer + Idx + 1), *((UINT8 *)Buffer + Idx + 2),
	    *((UINT8 *)Buffer + Idx + 3), *((UINT8 *)Buffer + Idx + 4), *((UINT8 *)Buffer + Idx + 5),
	    *((UINT8 *)Buffer + Idx + 6), *((UINT8 *)Buffer + Idx + 7));
    SerialPortWrite ((UINT8 *) CBuffer, CharCount);
  }
  DEBUG ((EFI_D_ERROR, "TBB:%x, TCB:%x, BYTCNT:%x, BLKSIZ:%x\n", MmioRead32 (DWSD_TBBCNT), MmioRead32 (DWSD_TCBCNT), MmioRead32 (DWSD_BYTCNT), MmioRead32 (DWSD_BLKSIZ)));
#endif
out:
  UncachedFreePages (IdmacDesc, DescPages);
  return Status;
}
#endif

#ifdef FIFO
EFI_STATUS
DwSdWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  return EFI_SUCCESS;
}
#else
EFI_STATUS
DwSdWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  DWSD_IDMAC_DESCRIPTOR*  IdmacDesc;
  EFI_STATUS  Status;
  UINT32      DescPages, CountPerPage, Count;

  CountPerPage = EFI_PAGE_SIZE / 16;
  Count = (Length + DWSD_DMA_BUF_SIZE - 1) / DWSD_DMA_BUF_SIZE;
  DescPages = (Count + CountPerPage - 1) / CountPerPage;
  IdmacDesc = (DWSD_IDMAC_DESCRIPTOR *)UncachedAllocatePages (DescPages);
  if (IdmacDesc == NULL)
    return EFI_BUFFER_TOO_SMALL;

  WriteBackDataCacheRange (Buffer, Length);

  Status = PrepareDmaData (IdmacDesc, Length, Buffer);
  if (EFI_ERROR (Status))
    goto out;

  Status = SendCommand (mDwSdCommand, mDwSdArgument);
out:
  UncachedFreePages (IdmacDesc, DescPages);
  return Status;
}
#endif

EFI_STATUS
DwSdSetIos (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN  UINT32                    BusClockFreq,
  IN  UINT32                    BusWidth,
  IN  UINT32                    TimingMode
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32    Data;

  if (TimingMode != EMMCBACKWARD) {
    Data = MmioRead32 (DWSD_UHSREG);
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
    MmioWrite32 (DWSD_UHSREG, Data);
  }

  switch (BusWidth) {
  case 1:
    MmioWrite32 (DWSD_CTYPE, 0);
    break;
  case 4:
    MmioWrite32 (DWSD_CTYPE, 1);
    break;
  case 8:
    MmioWrite32 (DWSD_CTYPE, 1 << 16);
    break;
  default:
    return EFI_UNSUPPORTED;
  }
  if (BusClockFreq) {
    Status = DwSdSetClock (BusClockFreq);
  }
  return Status;
}

EFI_MMC_HOST_PROTOCOL gMciHost = {
  MMC_HOST_PROTOCOL_REVISION,
  DwSdIsCardPresent,
  DwSdIsReadOnly,
  DwSdIsDmaSupported,
  DwSdBuildDevicePath,
  DwSdNotifyState,
  DwSdSendCommand,
  DwSdReceiveResponse,
  DwSdReadBlockData,
  DwSdWriteBlockData,
  DwSdSetIos
};

EFI_STATUS
DwSdDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  Handle = NULL;

  DEBUG ((EFI_D_BLKIO, "DwSdDxeInitialize()\n"));

  //Publish Component Name, BlockIO protocol interfaces
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiMmcHostProtocolGuid,         &gMciHost,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
