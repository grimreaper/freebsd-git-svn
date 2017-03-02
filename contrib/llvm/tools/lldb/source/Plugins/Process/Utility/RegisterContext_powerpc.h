//===-- RegisterContext_powerpc.h --------------------------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_RegisterContext_powerpc_H_
#define liblldb_RegisterContext_powerpc_H_

// eh_frame and DWARF Register numbers (eRegisterKindEHFrame &
// eRegisterKindDWARF)
enum {
  dwarf_r0_powerpc = 0,
  dwarf_r1_powerpc,
  dwarf_r2_powerpc,
  dwarf_r3_powerpc,
  dwarf_r4_powerpc,
  dwarf_r5_powerpc,
  dwarf_r6_powerpc,
  dwarf_r7_powerpc,
  dwarf_r8_powerpc,
  dwarf_r9_powerpc,
  dwarf_r10_powerpc,
  dwarf_r11_powerpc,
  dwarf_r12_powerpc,
  dwarf_r13_powerpc,
  dwarf_r14_powerpc,
  dwarf_r15_powerpc,
  dwarf_r16_powerpc,
  dwarf_r17_powerpc,
  dwarf_r18_powerpc,
  dwarf_r19_powerpc,
  dwarf_r20_powerpc,
  dwarf_r21_powerpc,
  dwarf_r22_powerpc,
  dwarf_r23_powerpc,
  dwarf_r24_powerpc,
  dwarf_r25_powerpc,
  dwarf_r26_powerpc,
  dwarf_r27_powerpc,
  dwarf_r28_powerpc,
  dwarf_r29_powerpc,
  dwarf_r30_powerpc,
  dwarf_r31_powerpc,
  dwarf_f0_powerpc,
  dwarf_f1_powerpc,
  dwarf_f2_powerpc,
  dwarf_f3_powerpc,
  dwarf_f4_powerpc,
  dwarf_f5_powerpc,
  dwarf_f6_powerpc,
  dwarf_f7_powerpc,
  dwarf_f8_powerpc,
  dwarf_f9_powerpc,
  dwarf_f10_powerpc,
  dwarf_f11_powerpc,
  dwarf_f12_powerpc,
  dwarf_f13_powerpc,
  dwarf_f14_powerpc,
  dwarf_f15_powerpc,
  dwarf_f16_powerpc,
  dwarf_f17_powerpc,
  dwarf_f18_powerpc,
  dwarf_f19_powerpc,
  dwarf_f20_powerpc,
  dwarf_f21_powerpc,
  dwarf_f22_powerpc,
  dwarf_f23_powerpc,
  dwarf_f24_powerpc,
  dwarf_f25_powerpc,
  dwarf_f26_powerpc,
  dwarf_f27_powerpc,
  dwarf_f28_powerpc,
  dwarf_f29_powerpc,
  dwarf_f30_powerpc,
  dwarf_f31_powerpc,
  dwarf_cr_powerpc,
  dwarf_fpscr_powerpc,
  dwarf_msr_powerpc,
  dwarf_vscr_powerpc,
  dwarf_xer_powerpc = 101,
  dwarf_lr_powerpc = 108,
  dwarf_ctr_powerpc,
  dwarf_pc_powerpc,
  dwarf_vrsave_powerpc = 356,
  dwarf_v0_powerpc = 1124,
  dwarf_v1_powerpc,
  dwarf_v2_powerpc,
  dwarf_v3_powerpc,
  dwarf_v4_powerpc,
  dwarf_v5_powerpc,
  dwarf_v6_powerpc,
  dwarf_v7_powerpc,
  dwarf_v8_powerpc,
  dwarf_v9_powerpc,
  dwarf_v10_powerpc,
  dwarf_v11_powerpc,
  dwarf_v12_powerpc,
  dwarf_v13_powerpc,
  dwarf_v14_powerpc,
  dwarf_v15_powerpc,
  dwarf_v16_powerpc,
  dwarf_v17_powerpc,
  dwarf_v18_powerpc,
  dwarf_v19_powerpc,
  dwarf_v20_powerpc,
  dwarf_v21_powerpc,
  dwarf_v22_powerpc,
  dwarf_v23_powerpc,
  dwarf_v24_powerpc,
  dwarf_v25_powerpc,
  dwarf_v26_powerpc,
  dwarf_v27_powerpc,
  dwarf_v28_powerpc,
  dwarf_v29_powerpc,
  dwarf_v30_powerpc,
  dwarf_v31_powerpc,
};

#endif // liblldb_RegisterContext_powerpc_H_
