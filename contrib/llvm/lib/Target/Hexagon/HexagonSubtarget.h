//===-- HexagonSubtarget.h - Define Subtarget for the Hexagon ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Hexagon specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef Hexagon_SUBTARGET_H
#define Hexagon_SUBTARGET_H

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "HexagonGenSubtargetInfo.inc"

#define Hexagon_SMALL_DATA_THRESHOLD 8
#define Hexagon_SLOTS 4

namespace llvm {

class HexagonSubtarget : public HexagonGenSubtargetInfo {

  bool UseMemOps;
  bool ModeIEEERndNear;

public:
  enum HexagonArchEnum {
    V1, V2, V3, V4, V5
  };

  HexagonArchEnum HexagonArchVersion;
  std::string CPUString;
  InstrItineraryData InstrItins;

public:
  HexagonSubtarget(StringRef TT, StringRef CPU, StringRef FS);

  /// getInstrItins - Return the instruction itineraies based on subtarget
  /// selection.
  const InstrItineraryData &getInstrItineraryData() const { return InstrItins; }


  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef FS);

  bool hasV2TOps () const { return HexagonArchVersion >= V2; }
  bool hasV2TOpsOnly () const { return HexagonArchVersion == V2; }
  bool hasV3TOps () const { return HexagonArchVersion >= V3; }
  bool hasV3TOpsOnly () const { return HexagonArchVersion == V3; }
  bool hasV4TOps () const { return HexagonArchVersion >= V4; }
  bool hasV4TOpsOnly () const { return HexagonArchVersion == V4; }
  bool useMemOps () const { return HexagonArchVersion >= V4 && UseMemOps; }
  bool hasV5TOps () const { return HexagonArchVersion >= V5; }
  bool hasV5TOpsOnly () const { return HexagonArchVersion == V5; }
  bool modeIEEERndNear () const { return ModeIEEERndNear; }

  bool isSubtargetV2() const { return HexagonArchVersion == V2;}
  const std::string &getCPUString () const { return CPUString; }

  // Threshold for small data section
  unsigned getSmallDataThreshold() const {
    return Hexagon_SMALL_DATA_THRESHOLD;
  }
  const HexagonArchEnum &getHexagonArchVersion() const {
    return  HexagonArchVersion;
  }
};

} // end namespace llvm

#endif
