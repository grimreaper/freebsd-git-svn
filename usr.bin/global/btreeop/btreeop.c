/*
 * Copyright (c) 1996, 1997 Shigio Yamaguchi. All rights reserved.
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
 *	This product includes software developed by Shigio Yamaguchi.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	btreeop.c				5-Apr-97
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <db.h>
#include <fcntl.h>

char    *dbdefault = "btree";   	/* default database name */
char    *progname  = "btreeop";	   	/* command name */
char    *dbname;
char	buf[BUFSIZ+1];

#ifndef __P
#if defined(__STDC__)
#define __P(protos)     protos
#else
#define __P(protos)     ()
#endif
#endif
void	die __P((char *));
void	usage __P((void));
void	entab __P((char *));
void	main __P((int, char **));
int	dbcreate __P((DB *));
int	dbkey __P((DB *, char *));
int	dbscan __P((DB *));

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN   1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN      4321
#endif

void
die(s)
char	*s;
{
	fprintf(stderr, "%s: %s\n", progname, s);
	exit(1);
}

void
usage() {
	fprintf(stderr,
		"usage: %s [-C][-K key][-b][-c cachesize][-l][-p psize][dbname]\n",
		progname);
	exit(1);
}

#define TABPOS(i)	((i)%8 == 0)
/*
 * entab: convert spaces into tabs
 *
 *	io)	buf	string buffer
 */
void
entab(buf)
char	*buf;
{
	int	blanks = 0;
	int	pos, src, dst;
	char	c;

	pos = src = dst = 0;
	while ((c = buf[src++]) != 0) {
		if (c == ' ') {
			if (!TABPOS(++pos)) {
				blanks++;		/* count blanks */
				continue;
			}
			buf[dst++] = '\t';
		} else if (c == '\t') {
			while (!TABPOS(++pos))
				;
			buf[dst++] = '\t';
		} else {
			++pos;
			while (blanks--)
				buf[dst++] = ' ';
			buf[dst++] = c;
		}
		blanks = 0;
	}
	buf[dst] = 0;
}

#include <errno.h>
void
main(argc, argv)
int	argc;
char	*argv[];
{
	char	command = 0;
	char	*key = NULL;
	DB	*db;
	BTREEINFO info;
	int	c;

	info.flags = R_DUP;		/* allow duplicate entries */
	info.cachesize = 500000;
	info.maxkeypage = 0;
	info.minkeypage = 0;
	info.psize = 0;
	info.compare = NULL;
	info.prefix = NULL;
	info.lorder = LITTLE_ENDIAN;

	while ((c = getopt(argc, argv, "CK:bc:lp:")) != EOF) {
		switch (c) {
		case 'K':
			key = optarg;
		case 'C':
			command = c;
			break;
		case 'b':
			info.lorder = BIG_ENDIAN;
			break;
		case 'c':
			info.cachesize = atoi(optarg);
			break;
		case 'l':
			info.lorder = LITTLE_ENDIAN;
			break;
		case 'p':
			info.psize = atoi(optarg);
			break;
		default:
			usage();
		}
	}

	dbname = (optind < argc) ? argv[optind] : dbdefault;
	db = dbopen(dbname, command == 'C' ? 
			O_RDWR|O_CREAT|O_TRUNC : O_RDONLY,
			0644, DB_BTREE, &info);

	if (db == NULL) {
		die("dbopen failed.");
	}
	switch (command) {
	case 'C':			/* Create database */
		dbcreate(db);
		break;
	case 'K':			/* Keyed search */
		dbkey(db, key);
		break;
	default:			/* Scan all data */
		dbscan(db);
		break;
	}
	if (db->close(db)) {
		die("db->close failed.");
	}
	exit(0);
}
/*
 * dbcreate: create database
 *
 *	i)	db
 *	r)		0: normal
 */
int
dbcreate(db)
DB	*db;
{
	DBT     key, dat;
	int	status;
#define IDENTLEN 80
	char	keybuf[IDENTLEN+1];
	char	*c;

	/*
	 * Input file format:
	 * +------------------
	 * |Key		Data\n
	 * |Key		Data\n
	 * 	.
	 * 	.
	 * - Key and Data are separated by blank('\t' or ' '). 
	 * - Key cannot include blank.
	 * - Data can include blank.
	 * - Null Data not allowed.
	 */
	while (fgets(buf, BUFSIZ, stdin)) {
		if (buf[strlen(buf)-1] == '\n')		/* chop(buf) */
			buf[strlen(buf)-1] = 0;
		else
			while (fgetc(stdin) != '\n')
				;
		for (c = buf; *c && !isspace(*c); c++)	/* skip key part */
			;
		if (*c == 0)
			die("data part not found.");
		if (c - buf > IDENTLEN)
			die("key too long.");
		strncpy(keybuf, buf, c - buf);		/* make key string */
		keybuf[c - buf] = 0;
		for (; *c && isspace(*c); c++)		/* skip blanks */
			;
		if (*c == 0)
			die("data part is null.");
		entab(buf);
		key.data = keybuf;
		key.size = strlen(keybuf)+1;
		dat.data = buf;
		dat.size = strlen(buf)+1;

		status = (db->put)(db, &key, &dat, 0);
		switch (status) {
		case RET_SUCCESS:
			break;
		case RET_ERROR:
		case RET_SPECIAL:
			die("db->put: failed.");
		}
	}
	return(0);
}

/*
 * dbkey: Keyed search
 *
 *	i)	db
 *	i)	skey	
 *	r)		0: normal
 *			1: not found
 */
int
dbkey(db, skey)
DB	*db;
char	*skey;
{
	DBT	dat, key;
	int	status;

	key.data = skey;
	key.size = strlen(skey)+1;

	for (status = (*db->seq)(db, &key, &dat, R_CURSOR);
		status == RET_SUCCESS && !strcmp(key.data, skey);
		status = (*db->seq)(db, &key, &dat, R_NEXT)) {
		(void)fprintf(stdout, "%s\n", (char *)dat.data);
	}
		
	if (status == RET_ERROR)
		die("db->seq failed.");
	return (0);
}

/*
 * dbscan: Scan all data
 *
 *	i)	db
 *	r)		0: normal
 *			1: not found
 */
int
dbscan(db)
DB	*db;
{
	DBT	dat, key;
	int	status;

	for (status = (*db->seq)(db, &key, &dat, R_FIRST);
		status == RET_SUCCESS;
		status = (*db->seq)(db, &key, &dat, R_NEXT)) {
		(void)fprintf(stdout, "%s\n", (char *)dat.data);
	}
	if (status == RET_ERROR)
		die("db->seq failed.");
	return (0);
}
