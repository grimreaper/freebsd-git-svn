/*-
 * Copyright (c) 1987, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)endian.h	8.1 (Berkeley) 6/10/93
 *	$NetBSD: endian.h,v 1.5 1997/10/09 15:42:19 bouyer Exp $
 * $FreeBSD$
 */

#ifndef _MACHINE_ENDIAN_H_
#define	_MACHINE_ENDIAN_H_

#include <machine/ansi.h>

/*
 * Define the order of 32-bit words in 64-bit words.
 */
#define _QUAD_HIGHWORD 1
#define _QUAD_LOWWORD 0

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#ifndef _POSIX_SOURCE
#define	LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long */

#define	BYTE_ORDER	LITTLE_ENDIAN
#endif /* !_POSIX_SOURCE */

#ifdef _KERNEL
#ifdef __GNUC__

#define _BSWAP32_DEFINED
static __inline __uint32_t
__bswap32(__uint32_t __x)
{
	__uint32_t __r;

	__asm __volatile__ (
		"insbl %1, 3, $1\n\t"
		"extbl %1, 1, $2\n\t"
		"extbl %1, 2, $3\n\t"
		"extbl %1, 3, $4\n\t"
		"sll $2, 16, $2\n\t"
		"sll $3, 8, $3\n\t"
		"or $4, $1, %0\n\t"
		"or $2, $3, $2\n\t"
		"or $2, %0, %0"
		: "=r" (__r) : "r" (__x) : "$1", "$2", "$3", "$4");
	return (__r);
}

#define _BSWAP16_DEFINED
static __inline __uint16_t 
__bswap16(__uint16_t __x)
{
	__uint16_t __r;

	__asm __volatile__ (
		"insbl %1, 1, $1\n\t"
		"extbl %1, 1, $2\n\t"
		"or $1, $2, %0"
		: "=r" (__r) : "r" (__x) : "$1", "$2");
	return (__r);
}

#endif /* _KERNEL */

#endif /* __GNUC__ */

#endif /* !_MACHINE_ENDIAN_H_ */
