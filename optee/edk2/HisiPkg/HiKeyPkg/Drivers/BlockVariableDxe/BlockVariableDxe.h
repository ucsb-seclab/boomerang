/** @file  NorFlashDxe.h

  Copyright (c) 2015, Linaro Ltd. All rights reserved.
  Copyright (c) 2015, Hisilicon Ltd. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VARIABLE_DXE_H__
#define __VARIABLE_DXE_H__

#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/FirmwareVolumeBlock.h>

#define BLOCK_VARIABLE_SIGNATURE                       SIGNATURE_32('b', 'l', 'k', '0')

typedef struct _BLOCK_VARIABLE_INSTANCE                BLOCK_VARIABLE_INSTANCE;

typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  EFI_DEVICE_PATH_PROTOCOL            End;
} BLOCK_DEVICE_PATH;

struct _BLOCK_VARIABLE_INSTANCE {
  UINT32                              Signature;
  EFI_HANDLE                          Handle;

  BOOLEAN                             Initialized;

  UINTN                               Size;
  EFI_LBA                             StartLba;

  EFI_BLOCK_IO_MEDIA                  Media;
  EFI_BLOCK_IO_PROTOCOL               *BlockIoProtocol;
  EFI_DISK_IO_PROTOCOL                DiskIoProtocol;
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL FvbProtocol;
  EFI_DEVICE_PATH_PROTOCOL            DevicePath;

  VOID*                               ShadowBuffer;
};


#endif /* __VARIABLE_DXE_H__ */
