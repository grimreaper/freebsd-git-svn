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
 * $FreeBSD$
 */

#define  MT_standout  1
#define  MT_delayed   2

#include <sys/time.h>
struct statics;

int		 display_updatecpus(struct statics *statics);
void	 clear_message(void);
int		 display_resize(void);
void	 i_header(const char *text);
char	*printable(char *string);
void	 display_header(int t);
int		 display_init(struct statics *statics);
void	 i_arc(int *stats);
void	 i_carc(int *stats);
void	 i_cpustates(int *states);
void	 i_loadave(int mpid, double *avenrun);
void	 i_memory(int *stats);
void	 i_message(void);
void	 i_process(int line, char *thisline);
void	 i_procstates(int total, int *brkdn);
void	 i_swap(int *stats);
void	 i_timeofday(time_t *tod);
void	 i_uptime(struct timeval *bt, time_t *tod);
void	 new_message(int type, const char *msgfmt, ...);
int	 readline(char *buffer, int size, int numeric);
const char	*trim_header(const char *text);
void	 u_arc(int *stats);
void	 u_carc(int *stats);
void	 u_cpustates(int *states);
void	 u_endscreen(int hi);
void	 u_header(const char *text);
void	 u_loadave(int mpid, double *avenrun);
void	 u_memory(int *stats);
void	 u_message(void);
void	 u_process(int line, char *newline);
void	 u_procstates(int total, int *brkdn);
void	 u_swap(int *stats);
void	 z_cpustates(void);
