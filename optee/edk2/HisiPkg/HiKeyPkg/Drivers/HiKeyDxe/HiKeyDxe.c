/** @file
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.
*  Copyright (c) 2015, Hisilicon Ltd. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseMemoryLib.h>
#include <Library/BdsLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/ArmGlobalVariableHob.h>

#include <Protocol/BlockIo.h>

#include "HiKeyDxeInternal.h"

#define SERIAL_NUMBER_LENGTH        16
#define SERIAL_NUMBER_LBA           1024
#define SERIAL_NUMBER_BLOCK_SIZE    512
#define RANDOM_MAGIC                0x9a4dbeaf

struct RandomSerialNo {
  UINTN              Magic;
  UINTN              Data;
  CHAR8              SerialNo[32];
};

STATIC
UINTN
EFIAPI
HiKeyInitSerialNo (
  IN   VOID
  )
{
  EFI_STATUS                      Status;
  UINTN                           VariableSize;
  EFI_DEVICE_PATH_PROTOCOL        *BlockDevicePath;
  EFI_BLOCK_IO_PROTOCOL           *BlockIoProtocol;
  EFI_HANDLE                      Handle;
  VOID                            *DataPtr;
  struct RandomSerialNo           *Random;
  CHAR16                          DefaultSerialNo[] = L"0123456789abcdef";
  CHAR16                          SerialNoUnicode[32], DataUnicode[32];

  BlockDevicePath = ConvertTextToDevicePath ((CHAR16*)FixedPcdGetPtr (PcdAndroidFastbootNvmDevicePath));
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &BlockDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: Couldn't locate block device (status: %r)\n", Status));
    return EFI_INVALID_PARAMETER;
  }
  Status = gBS->OpenProtocol (
		      Handle,
                      &gEfiBlockIoProtocolGuid,
		      (VOID **) &BlockIoProtocol,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: Couldn't open block device (status: %r)\n", Status));
    return EFI_DEVICE_ERROR;
  }
  DataPtr = AllocateZeroPool (SERIAL_NUMBER_BLOCK_SIZE);
  WriteBackDataCacheRange (DataPtr, SERIAL_NUMBER_BLOCK_SIZE);
  Status = BlockIoProtocol->ReadBlocks (BlockIoProtocol, BlockIoProtocol->Media->MediaId,
                                        SERIAL_NUMBER_LBA, SERIAL_NUMBER_BLOCK_SIZE,
                                        DataPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: failed on reading blocks.\n"));
    goto exit;
  }
  InvalidateDataCacheRange (DataPtr, SERIAL_NUMBER_BLOCK_SIZE);
  Random = (struct RandomSerialNo *)DataPtr;
  if (Random->Magic != RANDOM_MAGIC) {
    VariableSize = SERIAL_NUMBER_LENGTH * sizeof (CHAR16);
    Status = gRT->GetVariable (
                    (CHAR16 *)L"SerialNo",
                    &gArmGlobalVariableGuid,
                    NULL,
                    &VariableSize,
                    &DefaultSerialNo
                    );
    if (Status == EFI_NOT_FOUND) {
      Status = gRT->SetVariable (
                      (CHAR16*)L"SerialNo",
                      &gArmGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE       |
                      EFI_VARIABLE_BOOTSERVICE_ACCESS |
                      EFI_VARIABLE_RUNTIME_ACCESS,
                      VariableSize,
                      DefaultSerialNo
                      );
    }
  } else {
    AsciiStrToUnicodeStr (Random->SerialNo, SerialNoUnicode);
    VariableSize = SERIAL_NUMBER_LENGTH * sizeof (CHAR16);
    Status = gRT->GetVariable (
                    (CHAR16 *)L"SerialNo",
                    &gArmGlobalVariableGuid,
                    NULL,
                    &VariableSize,
                    &DataUnicode
                    );
    if ((Status == EFI_NOT_FOUND) || StrCmp (DataUnicode, SerialNoUnicode)) {
      Status = gRT->SetVariable (
                      (CHAR16*)L"SerialNo",
                      &gArmGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE       |
                      EFI_VARIABLE_BOOTSERVICE_ACCESS |
                      EFI_VARIABLE_RUNTIME_ACCESS,
                      VariableSize,
                      SerialNoUnicode
                      );
    }
  }
exit:
  FreePool (DataPtr);
  return Status;
}

STATIC
VOID
EFIAPI
HiKeyInitBootDevice (
  IN VOID
  )
{
  EFI_STATUS            Status;
  UINTN                 VariableSize;
  CHAR16                DefaultBootDevice[BOOT_DEVICE_LENGTH] = L"sd";

  VariableSize = BOOT_DEVICE_LENGTH * sizeof (CHAR16);
  Status = gRT->GetVariable (
                  (CHAR16 *)L"HiKeyBootDevice",
                  &gArmGlobalVariableGuid,
                  NULL,
                  &VariableSize,
                  &DefaultBootDevice
                  );
  if (Status == EFI_NOT_FOUND) {
    Status = gRT->SetVariable (
                    (CHAR16*)L"HiKeyBootDevice",
                    &gArmGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE       |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS,
                    VariableSize,
                    DefaultBootDevice
                    );
  }
}

EFI_STATUS
EFIAPI
HiKeyEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS           Status;

  HiKeyInitSerialNo ();
  HiKeyInitBootDevice ();
  HiKeyInitPeripherals ();

  Status = HiKeyBootMenuInstall ();

  return Status;
}
