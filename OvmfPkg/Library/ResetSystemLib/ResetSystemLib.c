/** @file
  Reset System Library functions for OVMF

  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <OvmfPlatforms.h>

VOID
AcpiPmControl (
  UINTN SuspendType
  )
{
  UINT16 AcpiPmBaseAddress;
  UINT16 HostBridgeDevId;

  ASSERT (SuspendType < 6);

  AcpiPmBaseAddress = 0;
  HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  switch (HostBridgeDevId) {
  case INTEL_82441_DEVICE_ID:
    AcpiPmBaseAddress = FixedPcdGet16 (PcdPiix4AcpiPmBaseAddress);
    break;
  case INTEL_Q35_MCH_DEVICE_ID:
    AcpiPmBaseAddress = FixedPcdGet16 (PcdIch9AcpiPmBaseAddress);
    break;
  default:
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  IoBitFieldWrite16 (AcpiPmBaseAddress + 4, 10, 13, (UINT16) SuspendType);
  IoOr16 (AcpiPmBaseAddress + 4, BIT13);
  CpuDeadLoop ();
}

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  IoWrite8 (0xCF9, BIT2 | BIT1); // 1st choice: PIIX3 RCR, RCPU|SRST
  MicroSecondDelay (50);

  IoWrite8 (0x64, 0xfe);         // 2nd choice: keyboard controller
  CpuDeadLoop ();
}

/**
  Calling this function causes a system-wide initialization. The processors
  are set to their initial state, and pending cycles are not corrupted.

  System reset should not return, if it returns, it means the system does
  not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  IoWrite8 (0x64, 0xfe);
  CpuDeadLoop ();
}

/**
  Calling this function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  System shutdown should not return, if it returns, it means the system does
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  AcpiPmControl (0);
  ASSERT (FALSE);
}


/**
  Calling this function causes the system to enter a power state for capsule
  update.

  Reset update should not return, if it returns, it means the system does
  not support capsule update.

**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  )
{
  AcpiPmControl (1);
  ASSERT (FALSE);
}
