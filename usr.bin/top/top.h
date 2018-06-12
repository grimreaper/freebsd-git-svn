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

#ifndef TOP_H
#define TOP_H

#include <unistd.h>

/* Number of lines of header information on the standard screen */
extern int Header_lines;

/* Maximum number of columns allowed for display */
#define MAX_COLS	512

/* Special atoi routine returns either a non-negative number or one of: */
#define Infinity	-1
#define Invalid		-2

/* maximum number we can have */
#define Largest		0x7fffffff

/* Exit code for system errors */
#define TOP_EX_SYS_ERROR	23

enum displaymodes { DISP_CPU = 0, DISP_IO, DISP_MAX };

/*
 * Format modifiers
 */
#define FMT_SHOWARGS 0x00000001

extern enum displaymodes displaymode;

extern int pcpu_stats;
extern int overstrike;
extern pid_t mypid;

extern const char * myname;

extern int (*compares[])(const void*, const void*);

const char* kill_procs(char *);
const char* renice_procs(char *);

extern char copyright[];

void quit(int);

/*
 *  The space command forces an immediate update.  Sometimes, on loaded
 *  systems, this update will take a significant period of time (because all
 *  the output is buffered).  So, if the short-term load average is above
 *  "LoadMax", then top will put the cursor home immediately after the space
 *  is pressed before the next update is attempted.  This serves as a visual
 *  acknowledgement of the command.
 */
#define LoadMax  5.0

/*
 *  "Nominal_TOPN" is used as the default TOPN when
 *  the output is a dumb terminal.  If we didn't do this, then
 *  we will get every
 *  process in the system when running top on a dumb terminal (or redirected
 *  to a file).  Note that Nominal_TOPN is a default:  it can still be
 *  overridden on the command line, even with the value "infinity".
 */
#define Nominal_TOPN	18

#endif /* TOP_H */
