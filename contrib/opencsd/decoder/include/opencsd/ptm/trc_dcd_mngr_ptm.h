/*
 * \file       trc_dcd_mngr_ptm.h
 * \brief      OpenCSD : PTM decoder manager / handler specialisation
 * 
 * \copyright  Copyright (c) 2016, ARM Limited. All Rights Reserved.
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
#ifndef ARM_TRC_DCD_MNGR_PTM_H_INCLUDED
#define ARM_TRC_DCD_MNGR_PTM_H_INCLUDED

#include "common/ocsd_dcd_mngr.h"
#include "trc_pkt_decode_ptm.h"
#include "trc_pkt_proc_ptm.h"
#include "trc_cmp_cfg_ptm.h"
#include "trc_pkt_types_ptm.h"

class DecoderMngrPtm : public DecodeMngrFullDcd< PtmTrcPacket, 
                                                 ocsd_ptm_pkt_type,
                                                 PtmConfig,
                                                 ocsd_ptm_cfg,
                                                 TrcPktProcPtm,
                                                 TrcPktDecodePtm>
{
public:
    DecoderMngrPtm(const std::string &name) : DecodeMngrFullDcd(name,OCSD_PROTOCOL_PTM) {};
    virtual ~DecoderMngrPtm() {};
};

#endif // ARM_TRC_DCD_MNGR_PTM_H_INCLUDED

/* End of File trc_dcd_mngr_ptm.h */
