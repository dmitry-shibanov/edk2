/** @file
  ACPI Platform Driver for ARM/AARCH64 virtual machines.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/QemuLoaderLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

  //
  // Our Depex ensures that this always succeeds.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid,
                  NULL /* Registration */, (VOID **)&AcpiTable);
  ASSERT_EFI_ERROR (Status);

  return InstallAllQemuLinkedTables (AcpiTable);
}
