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
 */

/*  This file contains the routines that interface to termcap and stty/gtty.
 *  Paul Vixie, February 1987: converted to use ioctl() instead of stty/gtty.
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termcap.h>
#include <termios.h>
#include <unistd.h>

#include "screen.h"
#include "top.h"

int  overstrike;
int  screen_length;
int  screen_width;
char ch_erase;
char ch_kill;
char smart_terminal;
static char termcap_buf[1024];
static char string_buffer[1024];
static char home[15];
static char lower_left[15];
char *tc_clear_line;
static char *tc_clear_screen;
char *tc_clear_to_end;
char *tc_cursor_motion;
static char *tc_start_standout;
static char *tc_end_standout;
static char *terminal_init;
static char *terminal_end;

static struct termios old_settings;
static struct termios new_settings;
static char is_a_terminal = false;

void
init_termcap(int interactive)
{
    char *bufptr;
    char *PCptr;
    char *term_name;
    int status;

    /* set defaults in case we aren't smart */
    screen_width = MAX_COLS;
    screen_length = 0;

    if (!interactive)
    {
	/* pretend we have a dumb terminal */
	smart_terminal = false;
	return;
    }

    /* assume we have a smart terminal until proven otherwise */
    smart_terminal = true;

    /* get the terminal name */
    term_name = getenv("TERM");

    /* if there is no TERM, assume it's a dumb terminal */
    /* patch courtesy of Sam Horrocks at telegraph.ics.uci.edu */
    if (term_name == NULL)
    {
	smart_terminal = false;
	return;
    }

    /* now get the termcap entry */
    if ((status = tgetent(termcap_buf, term_name)) != 1)
    {
	if (status == -1)
	{
	    fprintf(stderr, "%s: can't open termcap file\n", myname);
	}
	else
	{
	    fprintf(stderr, "%s: no termcap entry for a `%s' terminal\n",
		    myname, term_name);
	}

	/* pretend it's dumb and proceed */
	smart_terminal = false;
	return;
    }

    /* "hardcopy" immediately indicates a very stupid terminal */
    if (tgetflag("hc"))
    {
	smart_terminal = false;
	return;
    }

    /* set up common terminal capabilities */
    if ((screen_length = tgetnum("li")) <= 0)
    {
	screen_length = smart_terminal = 0;
	return;
    }

    /* screen_width is a little different */
    if ((screen_width = tgetnum("co")) == -1)
    {
	screen_width = 79;
    }
    else
    {
	screen_width -= 1;
    }

    /* terminals that overstrike need special attention */
    overstrike = tgetflag("os");

    /* initialize the pointer into the termcap string buffer */
    bufptr = string_buffer;

    /* get "ce", clear to end */
    if (!overstrike)
    {
	tc_clear_line = tgetstr("ce", &bufptr);
    }

    /* get necessary capabilities */
    if ((tc_clear_screen  = tgetstr("cl", &bufptr)) == NULL ||
	(tc_cursor_motion = tgetstr("cm", &bufptr)) == NULL)
    {
	smart_terminal = false;
	return;
    }

    /* get some more sophisticated stuff -- these are optional */
    tc_clear_to_end   = tgetstr("cd", &bufptr);
    terminal_init  = tgetstr("ti", &bufptr);
    terminal_end   = tgetstr("te", &bufptr);
    tc_start_standout = tgetstr("so", &bufptr);
    tc_end_standout   = tgetstr("se", &bufptr);

    /* pad character */
    PC = (PCptr = tgetstr("pc", &bufptr)) ? *PCptr : 0;

    /* set convenience strings */
    strncpy(home, tgoto(tc_cursor_motion, 0, 0), sizeof(home) - 1);
    home[sizeof(home) - 1] = '\0';
    /* (lower_left is set in get_screensize) */

    /* get the actual screen size with an ioctl, if needed */
    /* This may change screen_width and screen_length, and it always
       sets lower_left. */
    get_screensize();

    /* if stdout is not a terminal, pretend we are a dumb terminal */
    if (tcgetattr(STDOUT_FILENO, &old_settings) == -1)
    {
	smart_terminal = false;
    }
}

void
screen_init(void)
{
    /* get the old settings for safe keeping */
    if (tcgetattr(STDOUT_FILENO, &old_settings) != -1)
    {
	/* copy the settings so we can modify them */
	new_settings = old_settings;

	/* turn off ICANON, character echo and tab expansion */
	new_settings.c_lflag &= ~(ICANON|ECHO);
	new_settings.c_oflag &= ~(TAB3);
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &new_settings);

	/* remember the erase and kill characters */
	ch_erase = old_settings.c_cc[VERASE];
	ch_kill  = old_settings.c_cc[VKILL];

	/* remember that it really is a terminal */
	is_a_terminal = true;

	/* send the termcap initialization string */
	putcap(terminal_init);
    }

    if (!is_a_terminal)
    {
	/* not a terminal at all---consider it dumb */
	smart_terminal = false;
    }
}

void
end_screen(void)
{
    /* move to the lower left, clear the line and send "te" */
    if (smart_terminal)
    {
	putcap(lower_left);
	putcap(tc_clear_line);
	fflush(stdout);
	putcap(terminal_end);
    }

    /* if we have settings to reset, then do so */
    if (is_a_terminal)
    {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &old_settings);
    }
}

void
rescreen_init(void)
{
    /* install our settings if it is a terminal */
    if (is_a_terminal)
    {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &new_settings);
    }

    /* send init string */
    if (smart_terminal)
    {
	putcap(terminal_init);
    }
}

void
get_screensize(void)
{
    struct winsize ws;

    if (ioctl (1, TIOCGWINSZ, &ws) != -1)
    {
	if (ws.ws_row != 0)
	{
	    screen_length = ws.ws_row;
	}
	if (ws.ws_col != 0)
	{
	    screen_width = ws.ws_col - 1;
	}
    }


    strncpy(lower_left, tgoto(tc_cursor_motion, 0, screen_length - 1),
	sizeof(lower_left) - 1);
    lower_left[sizeof(lower_left) - 1] = '\0';
}

void
top_standout(const char *msg)
{
    if (smart_terminal)
    {
	putcap(tc_start_standout);
	fputs(msg, stdout);
	putcap(tc_end_standout);
    }
    else
    {
	fputs(msg, stdout);
    }
}

void
top_clear(void)
{
    if (smart_terminal)
    {
	putcap(tc_clear_screen);
    }
}

int
clear_eol(int len)
{
    if (smart_terminal && !overstrike && len > 0)
    {
	if (tc_clear_line)
	{
	    putcap(tc_clear_line);
	    return(0);
	}
	else
	{
	    while (len-- > 0)
	    {
		putchar(' ');
	    }
	    return(1);
	}
    }
    return(-1);
}

void
go_home(void)
{
    if (smart_terminal)
    {
	putcap(home);
    }
}
