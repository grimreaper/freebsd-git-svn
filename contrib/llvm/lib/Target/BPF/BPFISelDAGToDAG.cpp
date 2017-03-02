//===-- BPFISelDAGToDAG.cpp - A dag to dag inst selector for BPF ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a DAG pattern matching instruction selector for BPF,
// converting from a legalized dag to a BPF dag.
//
//===----------------------------------------------------------------------===//

#include "BPF.h"
#include "BPFRegisterInfo.h"
#include "BPFSubtarget.h"
#include "BPFTargetMachine.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

#define DEBUG_TYPE "bpf-isel"

// Instruction Selector Implementation
namespace {

class BPFDAGToDAGISel : public SelectionDAGISel {
public:
  explicit BPFDAGToDAGISel(BPFTargetMachine &TM) : SelectionDAGISel(TM) {}

  StringRef getPassName() const override {
    return "BPF DAG->DAG Pattern Instruction Selection";
  }

private:
// Include the pieces autogenerated from the target description.
#include "BPFGenDAGISel.inc"

  void Select(SDNode *N) override;

  // Complex Pattern for address selection.
  bool SelectAddr(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectFIAddr(SDValue Addr, SDValue &Base, SDValue &Offset);
};
}

// ComplexPattern used on BPF Load/Store instructions
bool BPFDAGToDAGISel::SelectAddr(SDValue Addr, SDValue &Base, SDValue &Offset) {
  // if Address is FI, get the TargetFrameIndex.
  SDLoc DL(Addr);
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base   = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
    Offset = CurDAG->getTargetConstant(0, DL, MVT::i64);
    return true;
  }

  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress)
    return false;

  // Addresses of the form Addr+const or Addr|const
  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
    if (isInt<32>(CN->getSExtValue())) {

      // If the first operand is a FI, get the TargetFI Node
      if (FrameIndexSDNode *FIN =
              dyn_cast<FrameIndexSDNode>(Addr.getOperand(0)))
        Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
      else
        Base = Addr.getOperand(0);

      Offset = CurDAG->getTargetConstant(CN->getSExtValue(), DL, MVT::i64);
      return true;
    }
  }

  Base   = Addr;
  Offset = CurDAG->getTargetConstant(0, DL, MVT::i64);
  return true;
}

// ComplexPattern used on BPF FI instruction
bool BPFDAGToDAGISel::SelectFIAddr(SDValue Addr, SDValue &Base, SDValue &Offset) {
  SDLoc DL(Addr);

  if (!CurDAG->isBaseWithConstantOffset(Addr))
    return false;

  // Addresses of the form Addr+const or Addr|const
  ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
  if (isInt<32>(CN->getSExtValue())) {

    // If the first operand is a FI, get the TargetFI Node
    if (FrameIndexSDNode *FIN =
            dyn_cast<FrameIndexSDNode>(Addr.getOperand(0)))
      Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
    else
      return false;

    Offset = CurDAG->getTargetConstant(CN->getSExtValue(), DL, MVT::i64);
    return true;
  }

  return false;
}

void BPFDAGToDAGISel::Select(SDNode *Node) {
  unsigned Opcode = Node->getOpcode();

  // Dump information about the Node being selected
  DEBUG(dbgs() << "Selecting: "; Node->dump(CurDAG); dbgs() << '\n');

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    DEBUG(dbgs() << "== "; Node->dump(CurDAG); dbgs() << '\n');
    return;
  }

  // tablegen selection should be handled here.
  switch (Opcode) {
  default: break;
  case ISD::SDIV: {
    DebugLoc Empty;
    const DebugLoc &DL = Node->getDebugLoc();
    if (DL != Empty)
      errs() << "Error at line " << DL.getLine() << ": ";
    else
      errs() << "Error: ";
    errs() << "Unsupport signed division for DAG: ";
    Node->dump(CurDAG);
    errs() << "Please convert to unsigned div/mod.\n";
    break;
  }
  case ISD::INTRINSIC_W_CHAIN: {
    unsigned IntNo = cast<ConstantSDNode>(Node->getOperand(1))->getZExtValue();
    switch (IntNo) {
    case Intrinsic::bpf_load_byte:
    case Intrinsic::bpf_load_half:
    case Intrinsic::bpf_load_word: {
      SDLoc DL(Node);
      SDValue Chain = Node->getOperand(0);
      SDValue N1 = Node->getOperand(1);
      SDValue Skb = Node->getOperand(2);
      SDValue N3 = Node->getOperand(3);

      SDValue R6Reg = CurDAG->getRegister(BPF::R6, MVT::i64);
      Chain = CurDAG->getCopyToReg(Chain, DL, R6Reg, Skb, SDValue());
      Node = CurDAG->UpdateNodeOperands(Node, Chain, N1, R6Reg, N3);
      break;
    }
    }
    break;
  }

  case ISD::FrameIndex: {
    int FI = cast<FrameIndexSDNode>(Node)->getIndex();
    EVT VT = Node->getValueType(0);
    SDValue TFI = CurDAG->getTargetFrameIndex(FI, VT);
    unsigned Opc = BPF::MOV_rr;
    if (Node->hasOneUse()) {
      CurDAG->SelectNodeTo(Node, Opc, VT, TFI);
      return;
    }
    ReplaceNode(Node, CurDAG->getMachineNode(Opc, SDLoc(Node), VT, TFI));
    return;
  }
  }

  // Select the default instruction
  SelectCode(Node);
}

FunctionPass *llvm::createBPFISelDag(BPFTargetMachine &TM) {
  return new BPFDAGToDAGISel(TM);
}
