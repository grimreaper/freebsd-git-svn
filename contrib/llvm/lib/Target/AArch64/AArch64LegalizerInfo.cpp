//===- AArch64LegalizerInfo.cpp ----------------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the Machinelegalizer class for
/// AArch64.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "AArch64LegalizerInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Target/TargetOpcodes.h"

using namespace llvm;

#ifndef LLVM_BUILD_GLOBAL_ISEL
#error "You shouldn't build this"
#endif

AArch64LegalizerInfo::AArch64LegalizerInfo() {
  using namespace TargetOpcode;
  const LLT p0 = LLT::pointer(0, 64);
  const LLT s1 = LLT::scalar(1);
  const LLT s8 = LLT::scalar(8);
  const LLT s16 = LLT::scalar(16);
  const LLT s32 = LLT::scalar(32);
  const LLT s64 = LLT::scalar(64);
  const LLT v2s32 = LLT::vector(2, 32);
  const LLT v4s32 = LLT::vector(4, 32);
  const LLT v2s64 = LLT::vector(2, 64);

  for (auto BinOp : {G_ADD, G_SUB, G_MUL, G_AND, G_OR, G_XOR, G_SHL}) {
    // These operations naturally get the right answer when used on
    // GPR32, even if the actual type is narrower.
    for (auto Ty : {s1, s8, s16, s32, s64, v2s32, v4s32, v2s64})
      setAction({BinOp, Ty}, Legal);
  }

  setAction({G_GEP, p0}, Legal);
  setAction({G_GEP, 1, s64}, Legal);

  for (auto Ty : {s1, s8, s16, s32})
    setAction({G_GEP, 1, Ty}, WidenScalar);

  for (auto BinOp : {G_LSHR, G_ASHR, G_SDIV, G_UDIV}) {
    for (auto Ty : {s32, s64})
      setAction({BinOp, Ty}, Legal);

    for (auto Ty : {s1, s8, s16})
      setAction({BinOp, Ty}, WidenScalar);
  }

  for (auto BinOp : { G_SREM, G_UREM })
    for (auto Ty : { s1, s8, s16, s32, s64 })
      setAction({BinOp, Ty}, Lower);

  for (auto Op : { G_UADDE, G_USUBE, G_SADDO, G_SSUBO, G_SMULO, G_UMULO }) {
    for (auto Ty : { s32, s64 })
      setAction({Op, Ty}, Legal);

    setAction({Op, 1, s1}, Legal);
  }

  for (auto BinOp : {G_FADD, G_FSUB, G_FMUL, G_FDIV})
    for (auto Ty : {s32, s64})
      setAction({BinOp, Ty}, Legal);

  setAction({G_FREM, s32}, Libcall);
  setAction({G_FREM, s64}, Libcall);

  for (auto MemOp : {G_LOAD, G_STORE}) {
    for (auto Ty : {s8, s16, s32, s64, p0, v2s32})
      setAction({MemOp, Ty}, Legal);

    setAction({MemOp, s1}, WidenScalar);

    // And everything's fine in addrspace 0.
    setAction({MemOp, 1, p0}, Legal);
  }

  // Constants
  for (auto Ty : {s32, s64}) {
    setAction({TargetOpcode::G_CONSTANT, Ty}, Legal);
    setAction({TargetOpcode::G_FCONSTANT, Ty}, Legal);
  }

  setAction({G_CONSTANT, p0}, Legal);

  for (auto Ty : {s1, s8, s16})
    setAction({TargetOpcode::G_CONSTANT, Ty}, WidenScalar);

  setAction({TargetOpcode::G_FCONSTANT, s16}, WidenScalar);

  setAction({G_ICMP, s1}, Legal);
  setAction({G_ICMP, 1, s32}, Legal);
  setAction({G_ICMP, 1, s64}, Legal);
  setAction({G_ICMP, 1, p0}, Legal);

  for (auto Ty : {s1, s8, s16}) {
    setAction({G_ICMP, 1, Ty}, WidenScalar);
  }

  setAction({G_FCMP, s1}, Legal);
  setAction({G_FCMP, 1, s32}, Legal);
  setAction({G_FCMP, 1, s64}, Legal);

  // Extensions
  for (auto Ty : { s1, s8, s16, s32, s64 }) {
    setAction({G_ZEXT, Ty}, Legal);
    setAction({G_SEXT, Ty}, Legal);
    setAction({G_ANYEXT, Ty}, Legal);
  }

  for (auto Ty : { s1, s8, s16, s32 }) {
    setAction({G_ZEXT, 1, Ty}, Legal);
    setAction({G_SEXT, 1, Ty}, Legal);
    setAction({G_ANYEXT, 1, Ty}, Legal);
  }

  setAction({G_FPEXT, s64}, Legal);
  setAction({G_FPEXT, 1, s32}, Legal);

  // Truncations
  for (auto Ty : { s16, s32 })
    setAction({G_FPTRUNC, Ty}, Legal);

  for (auto Ty : { s32, s64 })
    setAction({G_FPTRUNC, 1, Ty}, Legal);

  for (auto Ty : { s1, s8, s16, s32 })
    setAction({G_TRUNC, Ty}, Legal);

  for (auto Ty : { s8, s16, s32, s64 })
    setAction({G_TRUNC, 1, Ty}, Legal);

  // Conversions
  for (auto Ty : { s1, s8, s16, s32, s64 }) {
    setAction({G_FPTOSI, 0, Ty}, Legal);
    setAction({G_FPTOUI, 0, Ty}, Legal);
    setAction({G_SITOFP, 1, Ty}, Legal);
    setAction({G_UITOFP, 1, Ty}, Legal);
  }

  for (auto Ty : { s32, s64 }) {
    setAction({G_FPTOSI, 1, Ty}, Legal);
    setAction({G_FPTOUI, 1, Ty}, Legal);
    setAction({G_SITOFP, 0, Ty}, Legal);
    setAction({G_UITOFP, 0, Ty}, Legal);
  }

  // Control-flow
  for (auto Ty : {s1, s8, s16, s32})
    setAction({G_BRCOND, Ty}, Legal);

  // Select
  for (auto Ty : {s1, s8, s16, s32, s64})
    setAction({G_SELECT, Ty}, Legal);

  setAction({G_SELECT, 1, s1}, Legal);

  // Pointer-handling
  setAction({G_FRAME_INDEX, p0}, Legal);
  setAction({G_GLOBAL_VALUE, p0}, Legal);

  for (auto Ty : {s1, s8, s16, s32, s64})
    setAction({G_PTRTOINT, 0, Ty}, Legal);

  setAction({G_PTRTOINT, 1, p0}, Legal);

  setAction({G_INTTOPTR, 0, p0}, Legal);
  setAction({G_INTTOPTR, 1, s64}, Legal);

  // Casts for 32 and 64-bit width type are just copies.
  for (auto Ty : {s1, s8, s16, s32, s64}) {
    setAction({G_BITCAST, 0, Ty}, Legal);
    setAction({G_BITCAST, 1, Ty}, Legal);
  }

  // For the sake of copying bits around, the type does not really
  // matter as long as it fits a register.
  for (int EltSize = 8; EltSize <= 64; EltSize *= 2) {
    setAction({G_BITCAST, 0, LLT::vector(128/EltSize, EltSize)}, Legal);
    setAction({G_BITCAST, 1, LLT::vector(128/EltSize, EltSize)}, Legal);
    if (EltSize >= 64)
      continue;

    setAction({G_BITCAST, 0, LLT::vector(64/EltSize, EltSize)}, Legal);
    setAction({G_BITCAST, 1, LLT::vector(64/EltSize, EltSize)}, Legal);
    if (EltSize >= 32)
      continue;

    setAction({G_BITCAST, 0, LLT::vector(32/EltSize, EltSize)}, Legal);
    setAction({G_BITCAST, 1, LLT::vector(32/EltSize, EltSize)}, Legal);
  }

  computeTables();
}
