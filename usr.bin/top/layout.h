/*-
 * Copyright (c) 1984 through 2008, William LeFebvre
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 *     * Neither the name of William LeFebvre nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _LAYOUT_H
#define _LAYOUT_H

extern int  x_lastpid;		/* 10 */
extern int  y_lastpid;		/* 0 */
extern int  x_loadave;		/* 33 */
extern int  x_loadave_nompid;	/* 15 */
extern int  y_loadave;		/* 0 */
extern int  x_procstate;	/* 0 */
extern int  y_procstate;	/* 1 */
extern int  x_brkdn;		/* 15 */
extern int  y_brkdn;		/* 1 */
extern int  x_mem;		/* 5 */
extern int  y_mem;		/* 3 */
extern int  x_arc;		/* 5 */
extern int  y_arc;		/* 4 */
extern int  x_carc;		/* 5 */
extern int  y_carc;		/* 5 */
extern int  x_swap;		/* 6 */
extern int  y_swap;		/* 4 */
extern int  y_message;		/* 5 */
extern int  x_header;		/* 0 */
extern int  y_header;		/* 6 */
extern int  x_idlecursor;	/* 0 */
extern int  y_idlecursor;	/* 5 */
extern int  y_procs;		/* 7 */

extern int  y_cpustates;	/* 2 */

#endif
