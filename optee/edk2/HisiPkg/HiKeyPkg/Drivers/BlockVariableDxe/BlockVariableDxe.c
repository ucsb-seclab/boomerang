/** @file
  This file implement the Variable Protocol for the block device.

  Copyright (c) 2015, Linaro Limited. All rights reserved.
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>

#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "BlockVariableDxe.h"


STATIC EFI_PHYSICAL_ADDRESS      mMapNvStorageVariableBase;

EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL    *This,
  OUT       EFI_FVB_ATTRIBUTES_2                   *Attributes
  )
{
  EFI_FVB_ATTRIBUTES_2 FvbAttributes;
  FvbAttributes = (EFI_FVB_ATTRIBUTES_2) (

      EFI_FVB2_READ_ENABLED_CAP | // Reads may be enabled
      EFI_FVB2_READ_STATUS      | // Reads are currently enabled
      EFI_FVB2_STICKY_WRITE     | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
      EFI_FVB2_MEMORY_MAPPED    | // It is memory mapped
      EFI_FVB2_ERASE_POLARITY     // After erasure all bits take this value (i.e. '1')

      );
  FvbAttributes |= EFI_FVB2_WRITE_STATUS      | // Writes are currently enabled
                   EFI_FVB2_WRITE_ENABLED_CAP;  // Writes may be enabled

  *Attributes = FvbAttributes;

  DEBUG ((DEBUG_BLKIO, "FvbGetAttributes(0x%X)\n", *Attributes));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbSetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  DEBUG ((DEBUG_BLKIO, "FvbSetAttributes(0x%X) is not supported\n",*Attributes));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  )
{
  *Address = mMapNvStorageVariableBase;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  BLOCK_VARIABLE_INSTANCE       *Instance;

  Instance = CR (This, BLOCK_VARIABLE_INSTANCE, FvbProtocol, BLOCK_VARIABLE_SIGNATURE);
  *BlockSize = (UINTN) Instance->Media.BlockSize;
  *NumberOfBlocks = PcdGet32 (PcdNvStorageVariableBlockCount);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL   *This,
  IN        EFI_LBA                               Lba,
  IN        UINTN                                 Offset,
  IN OUT    UINTN                                 *NumBytes,
  IN OUT    UINT8                                 *Buffer
  )
{
  BLOCK_VARIABLE_INSTANCE       *Instance;
  EFI_BLOCK_IO_PROTOCOL         *BlockIo;
  EFI_STATUS                    Status;
  UINTN                         Bytes;
  VOID                          *DataPtr;

  Instance = CR (This, BLOCK_VARIABLE_INSTANCE, FvbProtocol, BLOCK_VARIABLE_SIGNATURE);
  BlockIo = Instance->BlockIoProtocol;
  Bytes = (Offset + *NumBytes + Instance->Media.BlockSize - 1) / Instance->Media.BlockSize * Instance->Media.BlockSize;
  DataPtr = AllocateZeroPool (Bytes);
  if (DataPtr == NULL) {
    DEBUG ((EFI_D_ERROR, "FvbWrite: failed to allocate buffer.\n"));
    return EFI_BUFFER_TOO_SMALL;
  }
  WriteBackDataCacheRange (DataPtr, Bytes);
  InvalidateDataCacheRange (Buffer, *NumBytes);
  Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId, Instance->StartLba + Lba,
		                Bytes, DataPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FvbRead StartLba:%x, Lba:%x, Offset:%x, Status:%x\n",
	    Instance->StartLba, Lba, Offset, Status));
    goto exit;
  }
  CopyMem (Buffer, DataPtr + Offset, *NumBytes);
  WriteBackDataCacheRange (Buffer, *NumBytes);
exit:
  FreePool (DataPtr);
  return Status;
}

EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL   *This,
  IN        EFI_LBA                               Lba,
  IN        UINTN                                 Offset,
  IN OUT    UINTN                                 *NumBytes,
  IN        UINT8                                 *Buffer
  )
{
  BLOCK_VARIABLE_INSTANCE       *Instance;
  EFI_BLOCK_IO_PROTOCOL         *BlockIo;
  EFI_STATUS                    Status;
  UINTN                         Bytes;
  VOID                          *DataPtr;

  Instance = CR (This, BLOCK_VARIABLE_INSTANCE, FvbProtocol, BLOCK_VARIABLE_SIGNATURE);
  BlockIo = Instance->BlockIoProtocol;
  Bytes = (Offset + *NumBytes + Instance->Media.BlockSize - 1) / Instance->Media.BlockSize * Instance->Media.BlockSize;
  DataPtr = AllocateZeroPool (Bytes);
  if (DataPtr == NULL) {
    DEBUG ((EFI_D_ERROR, "FvbWrite: failed to allocate buffer.\n"));
    return EFI_BUFFER_TOO_SMALL;
  }
  WriteBackDataCacheRange (DataPtr, Bytes);
  Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId, Instance->StartLba + Lba,
                                Bytes, DataPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FvbWrite: failed on reading blocks.\n"));
    goto exit;
  }
  CopyMem (DataPtr + Offset, Buffer, *NumBytes);
  WriteBackDataCacheRange (DataPtr, Bytes);
  Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, Instance->StartLba + Lba,
                                 Bytes, DataPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FvbWrite StartLba:%x, Lba:%x, Offset:%x, Status:%x\n",
            Instance->StartLba, Lba, Offset, Status));
  }
  // Sometimes the variable isn't flushed into block device if it's the last flush operation.
  // So flush it again.
  Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, Instance->StartLba + Lba,
                                 Bytes, DataPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FvbWrite StartLba:%x, Lba:%x, Offset:%x, Status:%x\n",
            Instance->StartLba, Lba, Offset, Status));
  }
exit:
  FreePool (DataPtr);
  return Status;
}

EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
  ...
  )
{
  return EFI_SUCCESS;
}

STATIC BLOCK_VARIABLE_INSTANCE   mBlockVariableInstance = {
  .Signature      = BLOCK_VARIABLE_SIGNATURE,
  .Media          = {
    .MediaId                       = 0,
    .RemovableMedia                = FALSE,
    .MediaPresent                  = TRUE,
    .LogicalPartition              = TRUE,
    .ReadOnly                      = FALSE,
    .WriteCaching                  = FALSE,
    .BlockSize                     = 0,
    .IoAlign                       = 4,
    .LastBlock                     = 0,
    .LowestAlignedLba              = 0,
    .LogicalBlocksPerPhysicalBlock = 0,
  },
  .FvbProtocol    = {
    .GetAttributes        = FvbGetAttributes,
    .SetAttributes        = FvbSetAttributes,
    .GetPhysicalAddress   = FvbGetPhysicalAddress,
    .GetBlockSize         = FvbGetBlockSize,
    .Read                 = FvbRead,
    .Write                = FvbWrite,
    .EraseBlocks          = FvbEraseBlocks,
  }
};

EFI_STATUS
ValidateFvHeader (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  )
{
  UINT16                      Checksum, TempChecksum;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINTN                       VariableStoreLength;
  UINTN                       FvLength;

  FvLength = (UINTN) (PcdGet32(PcdFlashNvStorageVariableSize) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
      PcdGet32(PcdFlashNvStorageFtwSpareSize));

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if (   (FwVolHeader->Revision  != EFI_FVH_REVISION)
      || (FwVolHeader->Signature != EFI_FVH_SIGNATURE)
      || (FwVolHeader->FvLength  != FvLength)
      )
  {
    DEBUG ((EFI_D_ERROR, "ValidateFvHeader: No Firmware Volume header present\n"));
    return EFI_NOT_FOUND;
  }

  // Check the Firmware Volume Guid
  if( CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid) == FALSE ) {
    DEBUG ((EFI_D_ERROR, "ValidateFvHeader: Firmware Volume Guid non-compatible\n"));
    return EFI_NOT_FOUND;
  }

  // Verify the header checksum
  TempChecksum = FwVolHeader->Checksum;
  FwVolHeader->Checksum = 0;
  Checksum = CalculateSum16((UINT16*)FwVolHeader, FwVolHeader->HeaderLength);
  if (Checksum != TempChecksum) {
    DEBUG ((EFI_D_ERROR, "ValidateFvHeader: FV checksum is invalid (Checksum:0x%X)\n",Checksum));
    return EFI_NOT_FOUND;
  }
  FwVolHeader->Checksum = Checksum;

  VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)FwVolHeader + FwVolHeader->HeaderLength);

  // Check the Variable Store Guid
  if( CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) == FALSE ) {
    DEBUG ((EFI_D_ERROR, "ValidateFvHeader: Variable Store Guid non-compatible\n"));
    return EFI_NOT_FOUND;
  }

  VariableStoreLength = PcdGet32 (PcdFlashNvStorageVariableSize) - FwVolHeader->HeaderLength;
  if (VariableStoreHeader->Size != VariableStoreLength) {
    DEBUG ((EFI_D_ERROR, "ValidateFvHeader: Variable Store Length does not match\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitNonVolatileVariableStore (
  IN BLOCK_VARIABLE_INSTANCE      *Instance,
  IN VOID                         *Headers,
  IN UINTN                        HeadersLength
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FirmwareVolumeHeader;
  EFI_STATUS                            Status;
  VARIABLE_STORE_HEADER                 *VariableStoreHeader;

  // Check if the size of the area is at least one block size
  ASSERT((PcdGet32(PcdFlashNvStorageVariableSize) > 0) && (PcdGet32(PcdFlashNvStorageVariableSize) / Instance->BlockIoProtocol->Media->BlockSize > 0));
  ASSERT((PcdGet32(PcdFlashNvStorageFtwWorkingSize) > 0) && (PcdGet32(PcdFlashNvStorageFtwWorkingSize) / Instance->BlockIoProtocol->Media->BlockSize > 0));
  ASSERT((PcdGet32(PcdFlashNvStorageFtwSpareSize) > 0) && (PcdGet32(PcdFlashNvStorageFtwSpareSize) / Instance->BlockIoProtocol->Media->BlockSize > 0));

  //
  // EFI_FIRMWARE_VOLUME_HEADER
  //
  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Headers;
  CopyGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
  FirmwareVolumeHeader->FvLength =
      PcdGet32(PcdFlashNvStorageVariableSize) +
      PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
      PcdGet32(PcdFlashNvStorageFtwSpareSize);
  FirmwareVolumeHeader->Signature = EFI_FVH_SIGNATURE;
  FirmwareVolumeHeader->Attributes = (EFI_FVB_ATTRIBUTES_2) (
                                            EFI_FVB2_READ_ENABLED_CAP   | // Reads may be enabled
                                            EFI_FVB2_READ_STATUS        | // Reads are currently enabled
                                            EFI_FVB2_STICKY_WRITE       | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
                                            EFI_FVB2_MEMORY_MAPPED      | // It is memory mapped
                                            EFI_FVB2_ERASE_POLARITY     | // After erasure all bits take this value (i.e. '1')
                                            EFI_FVB2_WRITE_STATUS       | // Writes are currently enabled
                                            EFI_FVB2_WRITE_ENABLED_CAP    // Writes may be enabled
                                        );
  FirmwareVolumeHeader->HeaderLength          = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY);
  FirmwareVolumeHeader->Revision              = EFI_FVH_REVISION;
  FirmwareVolumeHeader->BlockMap[0].NumBlocks = PcdGet32 (PcdNvStorageVariableBlockCount);
  FirmwareVolumeHeader->BlockMap[0].Length    = Instance->BlockIoProtocol->Media->BlockSize;
  // BlockMap Terminator
  FirmwareVolumeHeader->BlockMap[1].NumBlocks = 0;
  FirmwareVolumeHeader->BlockMap[1].Length    = 0;
  FirmwareVolumeHeader->Checksum = 0;
  FirmwareVolumeHeader->Checksum = CalculateSum16 ((UINT16*)FirmwareVolumeHeader, FirmwareVolumeHeader->HeaderLength);

  //
  // VARIABLE_STORE_HEADER
  //
  VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)FirmwareVolumeHeader + FirmwareVolumeHeader->HeaderLength);
  CopyGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid);
  VariableStoreHeader->Size = PcdGet32(PcdFlashNvStorageVariableSize) - FirmwareVolumeHeader->HeaderLength;
  VariableStoreHeader->Format            = VARIABLE_STORE_FORMATTED;
  VariableStoreHeader->State             = VARIABLE_STORE_HEALTHY;

  Status = FvbWrite (&Instance->FvbProtocol, 0, 0, &HeadersLength, Headers);
  return Status;
}

EFI_STATUS
BlockVariableDxeInitialize (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_HANDLE                      Handle;
  EFI_STATUS                      Status;
  BLOCK_VARIABLE_INSTANCE         *Instance = &mBlockVariableInstance;
  UINT32                          Count;
  EFI_LBA                         Lba;
  UINTN                           NvStorageSize;
  EFI_DEVICE_PATH_PROTOCOL        *NvBlockDevicePath;
  UINT8                           *NvStorageData;
  VOID                            *Headers;
  UINTN                           HeadersLength;

  Instance->Signature = BLOCK_VARIABLE_SIGNATURE;

  HeadersLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY) + sizeof(VARIABLE_STORE_HEADER);
  Headers = AllocateZeroPool(HeadersLength);
  if (Headers == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: failed to allocate memory of Headers\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  Lba = (EFI_LBA) PcdGet32 (PcdNvStorageVariableBlockLba);
  Count = PcdGet32 (PcdNvStorageVariableBlockCount);
  Instance->Media.BlockSize = PcdGet32 (PcdNvStorageVariableBlockSize);
  NvStorageSize = Count * Instance->Media.BlockSize;
  Instance->StartLba = Lba;
  HeadersLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY) + sizeof(VARIABLE_STORE_HEADER);
  if (NvStorageSize < HeadersLength) {
    return EFI_BAD_BUFFER_SIZE;
  }
  NvStorageData = (UINT8 *) (UINTN) PcdGet32(PcdFlashNvStorageVariableBase);
  mMapNvStorageVariableBase = PcdGet32(PcdFlashNvStorageVariableBase);
  NvBlockDevicePath = &Instance->DevicePath;
  NvBlockDevicePath = ConvertTextToDevicePath ((CHAR16*)FixedPcdGetPtr (PcdNvStorageVariableBlockDevicePath));
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &NvBlockDevicePath,
                                  &Instance->Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: Couldn't locate NVM device (status: %r)\n", Status));
    return EFI_INVALID_PARAMETER;
  }
  Status = gBS->OpenProtocol (
		      Instance->Handle,
                      &gEfiBlockIoProtocolGuid,
		      (VOID **) &Instance->BlockIoProtocol,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: Couldn't open NVM device (status: %r)\n", Status));
    return EFI_DEVICE_ERROR;
  }
  WriteBackDataCacheRange (Instance, sizeof(BLOCK_VARIABLE_INSTANCE));

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
		  &Handle,
		  &gEfiFirmwareVolumeBlockProtocolGuid, &Instance->FvbProtocol,
		  NULL
		  );
  if (EFI_ERROR (Status)) {
    goto exit;
  }

  Status = FvbRead (&Instance->FvbProtocol, 0, 0, &HeadersLength, Headers);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateFvHeader (Headers);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a, Found invalid Fv Header\n", __func__));

    // Erase all the block device that is reserved for variable storage
    Status = FvbEraseBlocks (&Instance->FvbProtocol, (EFI_LBA)0, Count, EFI_LBA_LIST_TERMINATOR);
    if (EFI_ERROR (Status)) {
      goto exit;
    }

    Status = InitNonVolatileVariableStore (Instance, Headers, HeadersLength);
    if (EFI_ERROR (Status)) {
      goto exit;
    }
  }

  if (NvStorageSize > ((EFI_FIRMWARE_VOLUME_HEADER*)Headers)->FvLength) {
    NvStorageSize = ((EFI_FIRMWARE_VOLUME_HEADER*)Headers)->FvLength;
    NvStorageSize = ((NvStorageSize + Instance->Media.BlockSize - 1) / Instance->Media.BlockSize) * Instance->Media.BlockSize;
  }
  Status = FvbRead (&Instance->FvbProtocol, 0, 0, &NvStorageSize, NvStorageData);

exit:
  return Status;
}
