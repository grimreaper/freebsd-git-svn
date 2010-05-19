/*-
 * Copyright (c) 2001 Jake Burkholder <jake@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef	_MACHINE_RUNQ_H_
#define	_MACHINE_RUNQ_H_

#define	RQB_LEN		(1UL)		/* Number of priority status words. */
#define	RQB_L2BPW	(6UL)		/* Log2(sizeof(rqb_word_t) * NBBY)). */
#define	RQB_BPW		(1UL<<RQB_L2BPW)	/* Bits in an rqb_word_t. */

#define	RQB_BIT(pri)	(1UL << ((pri) & (RQB_BPW - 1)))
#define	RQB_WORD(pri)	((pri) >> RQB_L2BPW)

#define	RQB_FFS(word)	(__ffsl(word) - 1)

/*
 * Type of run queue status word.
 */
typedef	uint64_t	rqb_word_t;

static __inline uint64_t
__popcnt(uint64_t bits)
{
        uint64_t result;

	__asm __volatile("popcnt %0=%1" : "=r" (result) : "r" (bits));
	return result;
}


static __inline int
__ffsl(u_long mask)
{

	if (__predict_false(mask == 0ul))
		return (0);
	return (__popcnt(mask ^ (mask - 1)));
}

#endif
