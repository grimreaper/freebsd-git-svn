/*
 * Copyright (c) 2005 Apple Computer, Inc.
 * Copyright (c) 2005 SPARTA, Inc.
 * All rights reserved.
 *
 * This code was developed in part by Robert N. M. Watson, Senior Principal
 * Scientist, SPARTA, Inc.
 *
 * @APPLE_BSD_LICENSE_HEADER_START@
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @APPLE_BSD_LICENSE_HEADER_END@
 *
 * $P4: //depot/projects/trustedbsd/openbsm/bsm/audit_internal.h#7 $
 */

#ifndef _LIBBSM_INTERNAL_H
#define	_LIBBSM_INTERNAL_H

/*
 * audit_internal.h contains private interfaces that are shared by user space
 * and the kernel for the purposes of assembling audit records.  Applications
 * should not include this file or use the APIs found within, or it may be
 * broken with future releases of OpenBSM, which may delete, modify, or
 * otherwise break these interfaces or the assumptions they rely on.
 */

/* We could determined the header and trailer sizes by
 * defining appropriate structures. We hold off that approach
 * till we have a consistant way of using structures for all tokens.
 * This is not straightforward since these token structures may
 * contain pointers of whose contents we dont know the size
 * (e.g text tokens)
 */
#define	BSM_HEADER_SIZE		18
#define	BSM_TRAILER_SIZE	7

/*
 * BSM token streams store fields in big endian byte order, so as to be
 * portable; when encoding and decoding, we must convert byte orders for
 * typed values.
 */
#define	ADD_U_CHAR(loc, val)						\
	do {								\
		*(loc) = (val);						\
		(loc) += sizeof(u_char);				\
	} while(0)


#define	ADD_U_INT16(loc, val)						\
	do {								\
		be16enc((loc), (val));					\
		(loc) += sizeof(u_int16_t);				\
	} while(0)

#define	ADD_U_INT32(loc, val)						\
	do {								\
		be32enc((loc), (val));					\
		(loc) += sizeof(u_int32_t);				\
	} while(0)

#define	ADD_U_INT64(loc, val)						\
	do {								\
		be64enc((loc), (val));					\
		(loc) += sizeof(u_int64_t); 				\
	} while(0)

#define	ADD_MEM(loc, data, size)					\
	do {								\
		memcpy((loc), (data), (size));				\
		(loc) += size;						\
	} while(0)

#define	ADD_STRING(loc, data, size)	ADD_MEM(loc, data, size)

#endif /* !_LIBBSM_INTERNAL_H_ */
