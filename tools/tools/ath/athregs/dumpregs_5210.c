/*-
 * Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 * $FreeBSD$
 */
#include "diag.h"

#include "ah.h"
#include "ah_internal.h"
#include "ar5210/ar5210reg.h"
#include "ar5210/ar5210phy.h"

#include "dumpregs.h"

#define	N(a)	(sizeof(a) / sizeof(a[0]))

static struct dumpreg ar5210regs[] = {
    DEFBASIC(AR_TXDP0,		"TXDP0"),
    DEFBASIC(AR_TXDP1,		"TXDP1"),
    DEFBASICfmt(AR_CR,		"CR",		AR_CR_BITS),
    DEFBASIC(AR_RXDP,		"RXDP"),
    DEFBASICfmt(AR_CFG,		"CFG",		AR_CFG_BITS),
    /* NB: read clears pending interrupts */
    DEFVOIDfmt(AR_ISR,		"ISR",		AR_ISR_BITS),
    DEFBASICfmt(AR_IMR,		"IMR",		AR_IMR_BITS),
    DEFBASICfmt(AR_IER,		"IER",		AR_IER_BITS),
    DEFBASICfmt(AR_BCR,		"BCR",		AR_BCR_BITS),
    DEFBASICfmt(AR_BSR,		"BSR",		AR_BSR_BITS),
    DEFBASICfmt(AR_TXCFG,	"TXCFG",	AR_TXCFG_BITS),
    DEFBASIC(AR_RXCFG,		"RXCFG"),
    DEFBASIC(AR_MIBC,		"MIBC"),
    DEFBASIC(AR_TOPS,		"TOPS"),
    DEFBASIC(AR_RXNOFRM,	"RXNOFR"),
    DEFBASIC(AR_TXNOFRM,	"TXNOFR"),
    DEFBASIC(AR_RPGTO,		"RPGTO"),
    DEFBASIC(AR_RFCNT,		"RFCNT"),
    DEFBASIC(AR_MISC,		"MISC"),
    DEFBASICfmt(AR_RC,		"RC",		AR_RC_BITS),
    DEFBASICfmt(AR_SCR,		"SCR",		AR_SCR_BITS),
    DEFBASICfmt(AR_INTPEND,	"INTPEND",	AR_INTPEND_BITS),
    DEFBASIC(AR_SFR,		"SFR"),
    DEFBASICfmt(AR_PCICFG,	"PCICFG",	AR_PCICFG_BITS),
    DEFBASIC(AR_GPIOCR,		"GPIOCR"),
    DEFVOID(AR_GPIODO,		"GPIODO"),
    DEFVOID(AR_GPIODI,		"GPIODI"),
    DEFBASIC(AR_SREV,		"SREV"),
    DEFBASIC(AR_STA_ID0,	"STA_ID0"),
    DEFBASICfmt(AR_STA_ID1,	"STA_ID1",	AR_STA_ID1_BITS),
    DEFBASIC(AR_BSS_ID0,	"BSS_ID0"),
    DEFBASIC(AR_BSS_ID1,	"BSS_ID1"),
    DEFBASIC(AR_SLOT_TIME,	"SLOTTIME"),
    DEFBASIC(AR_TIME_OUT,	"TIME_OUT"),
    DEFBASIC(AR_RSSI_THR,	"RSSI_THR"),
    DEFBASIC(AR_RETRY_LMT,	"RETRY_LM"),
    DEFBASIC(AR_USEC,		"USEC"),
    DEFBASICfmt(AR_BEACON,		"BEACON",	AR_BEACON_BITS),
    DEFBASIC(AR_CFP_PERIOD,	"CFP_PER"),
    DEFBASIC(AR_TIMER0,		"TIMER0"),
    DEFBASIC(AR_TIMER1,		"TIMER1"),
    DEFBASIC(AR_TIMER2,		"TIMER2"),
    DEFBASIC(AR_TIMER3,		"TIMER3"),
    DEFBASIC(AR_IFS0,		"IFS0"),
    DEFBASIC(AR_IFS1,		"IFS1"	),
    DEFBASIC(AR_CFP_DUR,	"CFP_DUR"),
    DEFBASICfmt(AR_RX_FILTER,	"RXFILTER",	AR_BEACON_BITS),
    DEFBASIC(AR_MCAST_FIL0,	"MCAST_0"),
    DEFBASIC(AR_MCAST_FIL1,	"MCAST_1"),
    DEFBASIC(AR_TX_MASK0,	"TX_MASK0"),
    DEFBASIC(AR_TX_MASK1,	"TX_MASK1"),
    DEFVOID(AR_CLR_TMASK,	"CLR_TMASK"),
    DEFBASIC(AR_TRIG_LEV,	"TRIG_LEV"),
    DEFBASICfmt(AR_DIAG_SW,	"DIAG_SW",	AR_DIAG_SW_BITS),
    DEFBASIC(AR_TSF_L32,	"TSF_L32"),
    DEFBASIC(AR_TSF_U32,	"TSF_U32"),
    DEFBASIC(AR_LAST_TSTP,	"LAST_TST"),
    DEFBASIC(AR_RETRY_CNT,	"RETRYCNT"),
    DEFBASIC(AR_BACKOFF,	"BACKOFF"),
    DEFBASIC(AR_NAV,		"NAV"),
    DEFBASIC(AR_RTS_OK,		"RTS_OK"),
    DEFBASIC(AR_RTS_FAIL,	"RTS_FAIL"),
    DEFBASIC(AR_ACK_FAIL,	"ACK_FAIL"),
    DEFBASIC(AR_FCS_FAIL,	"FCS_FAIL"),
    DEFBASIC(AR_BEACON_CNT,	"BEAC_CNT"),

    DEFVOIDfmt(AR_PHY_FRCTL,	"PHY_FRCTL",	AR_PHY_FRCTL_BITS),
    DEFVOIDfmt(AR_PHY_AGC,	"PHY_AGC",	AR_PHY_AGC_BITS),
    DEFVOID(AR_PHY_CHIPID,	"PHY_CHIPID"),
    DEFVOIDfmt(AR_PHY_ACTIVE,	"PHY_ACTIVE",	AR_PHY_ACTIVE_BITS),
    DEFVOIDfmt(AR_PHY_AGCCTL,	"PHY_AGCCTL",	AR_PHY_AGCCTL_BITS),
};

static __constructor void
ar5210_ctor(void)
{
#define	MAC5210	SREV(1,0), SREV(2,0)
	register_regs(ar5210regs, N(ar5210regs), MAC5210, PHYANY);
	register_keycache(64, MAC5210, PHYANY);

	register_range(0x9800, 0x9840, DUMP_BASEBAND, MAC5210, PHYANY);
}
