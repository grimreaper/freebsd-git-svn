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
#ifndef COMMANDS_H
#define COMMANDS_H

void	show_errors(void);
int	error_count(void);
void	show_help(void);

enum cmd_id {
	CMD_NONE,
	CMD_redraw,
	CMD_update,
	CMD_quit,
	CMD_help,
	CMD_errors,
	CMD_number,
	CMD_delay,
	CMD_displays,
	CMD_kill,
	CMD_renice,
	CMD_idletog,
	CMD_user,
	CMD_selftog,
	CMD_thrtog,
	CMD_viewtog,
	CMD_viewsys,
	CMD_wcputog,
	CMD_showargs,
	CMD_jidtog,
	CMD_kidletog,
	CMD_pcputog,
	CMD_jail,
	CMD_swaptog,
	CMD_order,
	CMD_pid	,
	CMD_toggletid,
};

struct command {
	char c;
	const char * const desc;
	bool available_to_dumb;
	enum cmd_id id;
};

extern const struct command all_commands[];

#endif /* COMMANDS_H */
