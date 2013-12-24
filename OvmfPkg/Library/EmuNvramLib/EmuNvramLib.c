/** @file
  Library exposing OVMF's emulated NVRAM.

  Copyright (C) 2013, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/EmuNvramLib.h>
#include <Library/QemuFwCfgLib.h>

/**
  Detect if S3 support has been explicitly deactivated.

  @retval  TRUE  if S3 explicitly disabled,
  @retval  FALSE if firmware configuration unavailable, or S3 enabled.
*/
STATIC
BOOLEAN
EFIAPI
IsS3Disabled (
  VOID
  )
{
  //
  // Since this code can run in SEC, we must explicitly check for the
  // availability of the firmware configuration interface.
  //
  return QemuFwCfgIsAvailable () && QemuFwCfgS3Disabled ();
}


/**
  Return the size of the NVRAM portion used as LockBox.

  @retval  0 if LockBox inside the NVRAM is disabled.
             This includes the case when S3 has been explicitly disabled.
  @return  Size otherwise.
*/
UINT32
EFIAPI
EmuNvramLockBoxSize (
  VOID
  )
{
  if (IsS3Disabled ()) {
    return 0;
  }
  return PcdGet32 (PcdEmuNvramLockBoxSize);
}

/**
  Return the size of the NVRAM portion used for S3 Resume Pool emulation.

  @retval  0 if S3 Resume Pool emulation inside the NVRAM is disabled.
             This includes the case when S3 has been explicitly disabled.
  @return  Size otherwise.
*/
UINT32
EFIAPI
EmuNvramS3ResumePoolSize (
  VOID
  )
{
  if (IsS3Disabled ()) {
    return 0;
  }
  return PcdGet32 (PcdEmuNvramS3ResumePoolSize);
}

/**
  Return the full (cumulative) size of the emulated NVRAM.

  @retval  0 if NVRAM emulation is disabled.
             This includes the case when S3 has been explicitly disabled.
  @return  Size otherwise.
**/
UINT32
EFIAPI
EmuNvramSize (
  VOID
  )
{
  return ALIGN_VALUE (EmuNvramLockBoxSize (), EFI_PAGE_SIZE) +
         ALIGN_VALUE (EmuNvramS3ResumePoolSize (), EFI_PAGE_SIZE);
}

/**
  Return the base address of the emulated NVRAM.
**/
UINT32
EFIAPI
EmuNvramBase (
  VOID
  )
{
  return PcdGet32 (PcdOvmfMemFvBase) + PcdGet32 (PcdOvmfMemFvSize) + SIZE_4MB;
}

/**
  Return the base address of the NVRAM portion used as LockBox.
*/
UINT32
EFIAPI
EmuNvramLockBoxBase (
  VOID
  )
{
  return EmuNvramBase ();
}

/**
  Return the base address of the NVRAM portion used for S3 Resume Pool
  emulation.
*/
UINT32
EFIAPI
EmuNvramS3ResumePoolBase (
  VOID
  )
{
  return EmuNvramLockBoxBase () +
    ALIGN_VALUE (EmuNvramLockBoxSize (), EFI_PAGE_SIZE);
}
