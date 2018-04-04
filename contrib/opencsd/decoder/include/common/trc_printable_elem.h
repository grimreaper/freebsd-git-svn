/*!
 * \file       trc_printable_elem.h
 * \brief      OpenCSD : Standard printable element base class.
 * 
 * \copyright  Copyright (c) 2015, ARM Limited. All Rights Reserved.
 */


/* 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 * may be used to endorse or promote products derived from this software without 
 * specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */ 

#ifndef ARM_TRC_PRINTABLE_ELEM_H_INCLUDED
#define ARM_TRC_PRINTABLE_ELEM_H_INCLUDED

#include <string>
#include <cstdint>

/** @addtogroup ocsd_infrastructure
@{*/

/*!
 * @class trcPrintableElem
 * @brief Class to provide trace element strings for printing
 * 
 *  Provide a standard interface to the trace packet classes to allow the packets 
 *  to be printed in logging or tools.
 *
 *  Provides some standard formatting functionality
 * 
 */
class trcPrintableElem
{
public:
    trcPrintableElem() {};
    virtual ~trcPrintableElem() {};
    virtual void toString(std::string &str) const;
    virtual void toStringFmt(const uint32_t fmtFlags, std::string &str) const;

    // print formatting utilities
    static void getValStr(std::string &valStr, const int valTotalBitSize, const int valValidBits, const uint64_t value, const bool asHex = true, const int updateBits = 0);

};

inline void trcPrintableElem::toString(std::string &str) const
{
    str = "Trace Element : print not implemented";
}

inline void trcPrintableElem::toStringFmt(const uint32_t /*fmtFlags*/, std::string &str) const
{
    toString(str);
}

/** static template string function - used in "C" API to provide generic printing */
template<class Pc, class Pt>
void trcPrintElemToString(const void *p_pkt, std::string &str)
{
    Pc pktClass;
    pktClass = static_cast<const Pt *>(p_pkt);
    pktClass.toString(str);
}

/** @}*/

#endif // ARM_TRC_PRINTABLE_ELEM_H_INCLUDED

/* End of File trc_printable_elem.h */
