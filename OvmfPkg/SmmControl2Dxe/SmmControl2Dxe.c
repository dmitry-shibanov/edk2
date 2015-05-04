/** @file

  A DXE_RUNTIME_DRIVER providing synchronous SMI activations via the
  EFI_SMM_CONTROL2_PROTOCOL.

  We expect the PEI phase to have covered the following:
  - ensure that the underlying QEMU machine type be Q35
    (responsible: OvmfPkg/SmmAccess/SmmAccessPei.inf)
  - ensure that the ACPI PM IO space be configured
    (responsible: OvmfPkg/PlatformPei/PlatformPei.inf)

  Our own entry point is responsible for confirming the SMI feature and for
  configuring it.

  Copyright (C) 2013, 2015, Red Hat, Inc.<BR>
  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Q35MchIch9.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SmmControl2.h>

/**
  Invokes SMI activation from either the preboot or runtime environment.

  This function generates an SMI.

  @param[in]     This                The EFI_SMM_CONTROL2_PROTOCOL instance.
  @param[in,out] CommandPort         The value written to the command port.
  @param[in,out] DataPort            The value written to the data port.
  @param[in]     Periodic            Optional mechanism to engender a periodic
                                     stream.
  @param[in]     ActivationInterval  Optional parameter to repeat at this
                                     period one time or, if the Periodic
                                     Boolean is set, periodically.

  @retval EFI_SUCCESS            The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR       The timing is unsupported.
  @retval EFI_INVALID_PARAMETER  The activation period is unsupported.
  @retval EFI_INVALID_PARAMETER  The last periodic activation has not been
                                 cleared.
  @retval EFI_NOT_STARTED        The SMM base service has not been initialized.
**/
STATIC
EFI_STATUS
EFIAPI
SmmControl2DxeTrigger (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN OUT UINT8                        *CommandPort       OPTIONAL,
  IN OUT UINT8                        *DataPort          OPTIONAL,
  IN BOOLEAN                          Periodic           OPTIONAL,
  IN UINTN                            ActivationInterval OPTIONAL
  )
{
  //
  // No support for queued or periodic activation.
  //
  if (Periodic || ActivationInterval > 0) {
    return EFI_DEVICE_ERROR;
  }

  //
  // These steps are explained in detail in the entry point. We need to repeat
  // them here because SMI_LOCK doesn't lock APMC_EN.
  //
  IoOr32 (
    ((PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE)) & ICH9_PMBASE_MASK) +
     ICH9_PMBASE_OFS_SMI_EN),
    ICH9_SMI_EN_APMC_EN
    );

  //
  // The so-called "Advanced Power Management Status Port Register" is in fact
  // a generic data passing register, between the caller and the SMI
  // dispatcher. The ICH9 spec calls it "scratchpad register" --  calling it
  // "status" elsewhere seems quite the misnomer. Status registers usually
  // report about hardware status, while this register is fully governed by
  // software.
  //
  // Write to the status register first, as this won't trigger the SMI just
  // yet. Then write to the control register.
  //
  IoWrite8 (ICH9_APM_STS, DataPort    == NULL ? 0 : *DataPort);
  IoWrite8 (ICH9_APM_CNT, CommandPort == NULL ? 0 : *CommandPort);
  return EFI_SUCCESS;
}

/**
  Clears any system state that was created in response to the Trigger() call.

  This function acknowledges and causes the deassertion of the SMI activation
  source.

  @param[in] This                The EFI_SMM_CONTROL2_PROTOCOL instance.
  @param[in] Periodic            Optional parameter to repeat at this period
                                 one time

  @retval EFI_SUCCESS            The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR       The source could not be cleared.
  @retval EFI_INVALID_PARAMETER  The service did not support the Periodic input
                                 argument.
**/
STATIC
EFI_STATUS
EFIAPI
SmmControl2DxeClear (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN BOOLEAN                          Periodic OPTIONAL
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The PI spec v1.4 explains that Clear() is only supposed to clear software
  // status; it is not in fact responsible for deasserting the SMI. It gives
  // two reasons for this: (a) many boards clear the SMI automatically when
  // entering SMM, (b) if Clear() actually deasserted the SMI, then it could
  // incorrectly suppress an SMI that was asynchronously asserted between the
  // last return of the SMI handler and the call made to Clear().
  //
  // In fact QEMU automatically deasserts CPU_INTERRUPT_SMI in:
  // - x86_cpu_exec_interrupt() [target-i386/seg_helper.c], and
  // - kvm_arch_pre_run() [target-i386/kvm.c].
  //
  // So, nothing to do here.
  //
  return EFI_SUCCESS;
}

STATIC EFI_SMM_CONTROL2_PROTOCOL mControl2 = {
  &SmmControl2DxeTrigger,
  &SmmControl2DxeClear,
  MAX_UINTN // MinimumTriggerPeriod -- we don't support periodic SMIs
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
SmmControl2DxeEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  UINT32 PmBase;
  UINTN  SmiEnable;
  UINT32 SmiEnableVal;

  //
  // This module should only be included if SMRAM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  //
  // Calculate the absolute IO port address of the SMI Control and Enable
  // Register. (As noted at the top, the PEI phase has left us with a working
  // ACPI PM IO space.)
  //
  PmBase = PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE)) &
    ICH9_PMBASE_MASK;
  SmiEnable = PmBase + ICH9_PMBASE_OFS_SMI_EN;

  //
  // If APMC_EN is pre-set in SMI_EN, that's QEMU's way to tell us that SMI
  // support is not available. (For example due to KVM lacking it.) Otherwise,
  // this bit is clear after each reset.
  //
  SmiEnableVal = IoRead32 (SmiEnable);
  if ((SmiEnableVal & ICH9_SMI_EN_APMC_EN) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: this Q35 implementation lacks SMI\n",
      __FUNCTION__));
    goto WrongConfig;
  }

  //
  // Otherwise, configure the board to inject an SMI when ICH9_APM_CNT is
  // written to. (See the Trigger() method above.)
  //
  SmiEnableVal |= ICH9_SMI_EN_APMC_EN | ICH9_SMI_EN_GBL_SMI_EN;
  IoWrite32 (SmiEnable, SmiEnableVal);

  //
  // Prevent software from undoing the above (until platform reset).
  //
  PciOr16 (POWER_MGMT_REGISTER_Q35 (ICH9_GEN_PMCON_1),
    ICH9_GEN_PMCON_1_SMI_LOCK);

  //
  // If we can clear GBL_SMI_EN now, that means QEMU's SMI support is not
  // appropriate.
  //
#if 0
  IoWrite32 (SmiEnable, SmiEnableVal & ~(UINT32)ICH9_SMI_EN_GBL_SMI_EN);
  if (IoRead32 (SmiEnable) != SmiEnableVal) {
    DEBUG ((EFI_D_ERROR, "%a: failed to lock down GBL_SMI_EN\n",
      __FUNCTION__));
    goto WrongConfig;
  }
#endif

  //
  // We're done.
  //
  // We have no pointers to convert to virtual addresses. The handle itself
  // doesn't matter, as protocol services are not accessible at runtime.
  //
  return gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                &gEfiSmmControl2ProtocolGuid, &mControl2,
                NULL);

WrongConfig:
  //
  // We really don't want to continue in this case.
  //
  // _ASSERT() is never compiled out, and it respects PcdDebugPropertyMask (ie.
  // prevent further execution with CPU breakpoint vs. dead loop).
  //
  _ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
