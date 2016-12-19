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

#ifndef __HIKEY_DXE_INTERNAL_H__
#define __HIKEY_DXE_INTERNAL_H__

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define BOOT_DEVICE_LENGTH       16

EFI_STATUS
HiKeyFdtInstall (
  IN EFI_HANDLE                            ImageHandle
  );

EFI_STATUS
HiKeyBootMenuInstall (
  IN VOID
  );

EFI_STATUS
HiKeyInitPeripherals (
  IN VOID
  );

#endif // __HIKEY_DXE_INTERNAL_H__
