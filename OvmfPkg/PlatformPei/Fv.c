/** @file
  Build FV related hobs for platform.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiPei.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>


/**
  Perform a call-back into the SEC simulator to get address of the Firmware Hub

  @param  FfsHeader     Ffs Header availible to every PEIM
  @param  PeiServices   General purpose services available to every PEIM.

  @retval EFI_SUCCESS   Platform PEI FVs were initialized successfully.

**/
EFI_STATUS
PeiFvInitialization (
  VOID
  )
{
  DEBUG ((EFI_D_ERROR, "Platform PEI Firmware Volume Initialization\n"));

  DEBUG (
    (EFI_D_ERROR, "Firmware Volume HOB: 0x%x 0x%x\n",
      PcdGet32 (PcdOvmfMemFvBase),
      PcdGet32 (PcdOvmfMemFvSize)
      )
    );

  BuildFvHob (PcdGet32 (PcdOvmfMemFvBase), PcdGet32 (PcdOvmfMemFvSize));

  //
  // Cover the decompressed main firmware with a memory allocation that
  // prevents the OS from using it. At S3 resume we overwrite this area.
  //
  BuildMemoryAllocationHob (
    PcdGet32 (PcdOvmfMemFvBase),
    PcdGet32 (PcdOvmfMemFvSize),
    EfiACPIMemoryNVS
    );

  //
  // Firmware decompression in DecompressGuidedFv() [OvmfPkg/Sec/SecMain.c]
  // uses additional temporary memory.
  //
  BuildMemoryAllocationHob (
    PcdGet32 (PcdOvmfMemFvBase) + PcdGet32 (PcdOvmfMemFvSize),
    (SIZE_2MB  + // cover the end of OutputBuffer, rounded up to 1MB
     SIZE_64KB   // cover the end of ScratchBuffer
     ),
    EfiACPIMemoryNVS
    );

  //
  // Cover the initial RAM area used as stack and temporary PEI heap. The base
  // constant comes from OvmfPkg/Sec/{Ia32,X64}/SecEntry.{asm,S}, the size
  // originates from SecCoreStartupWithStack() [OvmfPkg/Sec/SecMain.c].
  //
  BuildMemoryAllocationHob (
    BASE_512KB - SIZE_64KB,
    SIZE_64KB,
    EfiACPIMemoryNVS
    );
  return EFI_SUCCESS;
}

