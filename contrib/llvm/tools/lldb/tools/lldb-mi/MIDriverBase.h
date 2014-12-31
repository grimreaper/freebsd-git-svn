//===-- MIDriverBase.h ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//++
// File:		MIDriverBase.h
//
// Overview:	CMIDriverBase interface.
//
// Environment:	Compilers:	Visual C++ 12.
//							gcc (Ubuntu/Linaro 4.8.1-10ubuntu9) 4.8.1
//				Libraries:	See MIReadmetxt. 
//
// Copyright:	None.
//--

#pragma once

// Third party headers:
#include <lldb/API/SBDebugger.h>
#include <lldb/API/SBBroadcaster.h>

// In-house headers:
#include "MIUtilString.h"

// Declarations:
namespace lldb { class SBBroadcaster; }

//++ ============================================================================
// Details:	MI driver base implementation class. This class has been created so 
//			not have to edit the lldb::SBBroadcaster class code. Functionality 
//			and attributes need to be common to the LLDB Driver class and the 
//			MI Driver class (derived from lldb::SBBroadcaster) so they can call 
//			upon each other for functionality fall through and allow the 
//			CDriverMgr to manage either (any) driver to be operated on.
//			Each driver instance (the CMIDriver, LLDB::Driver) has its own
//			LLDB::SBDebugger object.
// Gotchas:	None.
// Authors:	Illya Rudkin 30/01/2014.
// Changes:	None.
//--
class CMIDriverBase
{
// Methods:
public:
	/* ctor */	CMIDriverBase( void );
	
	CMIDriverBase *	GetDriverToFallThruTo( void ) const;
	CMIDriverBase *	GetDriversParent( void ) const;

// Overrideable:
public:
	/* dtor */ virtual ~CMIDriverBase( void );
	
	virtual bool					DoFallThruToAnotherDriver( const CMIUtilString & vCmd, CMIUtilString & vwErrMsg );
	virtual bool					SetDriverToFallThruTo( const CMIDriverBase & vrOtherDriver );
	virtual bool					SetDriverParent( const CMIDriverBase & vrOtherDriver );
	virtual const CMIUtilString &	GetDriverName( void ) const = 0;
	virtual const CMIUtilString &	GetDriverId( void ) const = 0;
	virtual void					SetExitApplicationFlag( const bool vbForceExit );
	
	// MI provide information for the pass through (child) assigned driver
	virtual FILE *	GetStdin( void ) const;		
	virtual FILE *	GetStdout( void ) const;	
	virtual FILE *	GetStderr( void ) const;
		
// Attributes:
protected:
	CMIDriverBase *	m_pDriverFallThru;	// Child driver to use should *this driver not be able to handle client input
	CMIDriverBase *	m_pDriverParent;	// The parent driver who passes work to *this driver to do work
	CMIUtilString	m_strDriverId;
	bool			m_bExitApp;			// True = Yes, exit application, false = continue execution
};
