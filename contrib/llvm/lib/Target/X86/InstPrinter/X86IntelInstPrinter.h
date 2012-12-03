//= X86IntelInstPrinter.h - Convert X86 MCInst to assembly syntax -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an X86 MCInst to Intel style .s file syntax.
//
//===----------------------------------------------------------------------===//

#ifndef X86_INTEL_INST_PRINTER_H
#define X86_INTEL_INST_PRINTER_H

#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

class MCOperand;
  
class X86IntelInstPrinter : public MCInstPrinter {
public:
  X86IntelInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                      const MCRegisterInfo &MRI)
    : MCInstPrinter(MAI, MII, MRI) {}

  virtual void printRegName(raw_ostream &OS, unsigned RegNo) const;
  virtual void printInst(const MCInst *MI, raw_ostream &OS, StringRef Annot);
  
  // Autogenerated by tblgen.
  void printInstruction(const MCInst *MI, raw_ostream &O);
  static const char *getRegisterName(unsigned RegNo);

  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printMemReference(const MCInst *MI, unsigned Op, raw_ostream &O);
  void printSSECC(const MCInst *MI, unsigned Op, raw_ostream &O);
  void printAVXCC(const MCInst *MI, unsigned Op, raw_ostream &O);
  void printPCRelImm(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  
  void printopaquemem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "OPAQUE PTR ";
    printMemReference(MI, OpNo, O);
  }
  
  void printi8mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "BYTE PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printi16mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "WORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printi32mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "DWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printi64mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "QWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printi128mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "XMMWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printi256mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "YMMWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printf32mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "DWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printf64mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "QWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printf80mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "XWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printf128mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "XMMWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
  void printf256mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "YMMWORD PTR ";
    printMemReference(MI, OpNo, O);
  }
};
  
}

#endif
