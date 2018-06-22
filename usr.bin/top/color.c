/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2018 Eitan Adler
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

#include <sys/param.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "color.h"

const color_t NO_COLOR = -1;

static const struct {
	const char * const name;
	enum tag_id tag;
} name_tag_map[] = {
	{ "min1", MIN_1 },
	{ "min5", MIN_5 },
	{ "min15", MIN_15 },
	{ NULL, MAX_TAG_ID }
};

struct tag_color_info colors[MAX_TAG_ID];

void
color_init(void) {
	char *env, *curcolor;
	char *tagsep, *colorsep, *minmaxsep, *bgfgsep;
	size_t i;
	enum tag_id whichtag;
	int min, max, fg, bg;

	for (i = 0; i < MAX_TAG_ID; ++i) {
		colors[i].foreground = NO_COLOR;
		colors[i].background = NO_COLOR;
		colors[i].min = 0;
		colors[i].max = 0;
	}

	env = getenv("TOPCOLOR");
	if (env == NULL) {
		return;
	}

	while ((curcolor = strsep(&env, ":")) != NULL) {
		/* this doesn't deal well with malformed strings
		 * but ideally shouldn't crash.
		 */

		/* First split into tag=spec*/
		tagsep = strchr(curcolor, '=');
		if (tagsep == NULL) {
			fprintf(stderr, "no tag found\n");
			exit(1);
		}
		*tagsep = 0;
		tagsep++;
		for (i = 0; name_tag_map[i].name != NULL; ++i) {
			if (strcmp(name_tag_map[i].name, curcolor) == 0) {
				whichtag = name_tag_map[i].tag;
				break;
			}
		}
		if (i+1 == nitems(name_tag_map)) {
			fprintf(stderr, "no such tag: %s\n", curcolor);
			exit(1);
		}


		/* starting from the spec separate into min,max and color */
		colorsep = strchr(tagsep, '#');
		colorsep++;
		if (colorsep == NULL) {
			fprintf(stderr, "invalid color: no # found\n");
			exit(1);
		}
		minmaxsep = strchr(tagsep, ',');
		minmaxsep++;
		if (minmaxsep == NULL || minmaxsep > colorsep) {
			fprintf(stderr, "invalid color: no min/max , found\n");
			exit(1);
		}
		min = (int)strtol(tagsep, NULL, 10);
		max = (int)strtol(minmaxsep, NULL, 10);

		bgfgsep = strchr(colorsep, ',');
		bgfgsep++;
		if (minmaxsep == NULL) {
			fprintf(stderr, "invalid color: no fg/bg , found\n");
			exit(1);
		}
		fg = (int)strtol(colorsep, NULL, 10);
		bg = (int)strtol(bgfgsep, NULL, 10);

		colors[whichtag].min = min;
		colors[whichtag].max = max;
		colors[whichtag].foreground = fg;
		colors[whichtag].background = bg;
	}
}
