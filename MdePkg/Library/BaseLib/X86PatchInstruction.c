/** @file
  IA-32/x64 PatchInstructionX86()

  Copyright (C) 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2018, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BaseLibInternals.h"

/**
  Patch the immediate operand of an IA32 or X64 instruction such that the byte,
  word, dword or qword operand is encoded at the end of the instruction's
  binary representation.

  This function should be used to update object code that was compiled with
  NASM from assembly source code. Example:

  NASM source code:

        mov     eax, strict dword 0 ; the imm32 zero operand will be patched
    ASM_PFX(gPatchCr3):
        mov     cr3, eax

  C source code:

    extern UINT8 gPatchCr3;
    PatchInstructionX86 (&gPatchCr3, AsmReadCr3 (), 4);

  @param[out] InstructionEnd  Pointer to the byte one past the instruction to
                              patch. The immediate operand to patch is expected
                              to comprise the trailing bytes of the
                              instruction. If InstructionEnd is closer to
                              address 0 than ValueSize permits, then ASSERT().

  @param[in] PatchValue       The constant to write to the immediate operand.
                              The caller is responsible for ensuring that
                              PatchValue can be represented in the byte, word,
                              dword or qword operand (as indicated through
                              ValueSize); otherwise ASSERT().

  @param[in] ValueSize        The size of the operand in bytes; must be 1, 2,
                              4, or 8. ASSERT() otherwise.
**/
VOID
EFIAPI
PatchInstructionX86 (
  OUT VOID   *InstructionEnd,
  IN  UINT64 PatchValue,
  IN  UINTN  ValueSize
  )
{
  //
  // The equality ((UINTN)InstructionEnd == ValueSize) would assume a zero-size
  // instruction at address 0; forbid it.
  //
  ASSERT ((UINTN)InstructionEnd > ValueSize);

  switch (ValueSize) {
  case 1:
    ASSERT (PatchValue <= MAX_UINT8);
    *((UINT8 *)InstructionEnd - 1) = (UINT8)PatchValue;
    break;

  case 2:
    ASSERT (PatchValue <= MAX_UINT16);
    WriteUnaligned16 ((UINT16 *)InstructionEnd - 1, (UINT16)PatchValue);
    break;

  case 4:
    ASSERT (PatchValue <= MAX_UINT32);
    WriteUnaligned32 ((UINT32 *)InstructionEnd - 1, (UINT32)PatchValue);
    break;

  case 8:
    WriteUnaligned64 ((UINT64 *)InstructionEnd - 1, PatchValue);
    break;

  default:
    ASSERT (FALSE);
  }
}
