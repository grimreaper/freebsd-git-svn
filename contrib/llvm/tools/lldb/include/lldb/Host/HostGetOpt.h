//===-- GetOpt.h ------------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#pragma once

#ifndef _MSC_VER

#ifdef _WIN32
#define _BSD_SOURCE // Required so that getopt.h defines optreset
#endif

#include <unistd.h>
#include <getopt.h>

#else

#include <lldb/Host/windows/getopt/GetOptInc.h>

#endif
