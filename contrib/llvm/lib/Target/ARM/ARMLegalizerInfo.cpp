//===- ARMLegalizerInfo.cpp --------------------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the Machinelegalizer class for ARM.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "ARMLegalizerInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetOpcodes.h"

using namespace llvm;

#ifndef LLVM_BUILD_GLOBAL_ISEL
#error "You shouldn't build this"
#endif

ARMLegalizerInfo::ARMLegalizerInfo() {
  using namespace TargetOpcode;

  const LLT p0 = LLT::pointer(0, 32);

  const LLT s8 = LLT::scalar(8);
  const LLT s16 = LLT::scalar(16);
  const LLT s32 = LLT::scalar(32);

  setAction({G_FRAME_INDEX, p0}, Legal);

  setAction({G_LOAD, s32}, Legal);
  setAction({G_LOAD, 1, p0}, Legal);

  for (auto Ty : {s8, s16, s32})
    setAction({G_ADD, Ty}, Legal);

  computeTables();
}
