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

/*
 *  Username translation code for top.
 *
 *  These routines handle uid to username mapping.
 *  They use a hashing table scheme to reduce reading overhead.
 *  For the time being, these are very straightforward hashing routines.
 *  Maybe someday I'll put in something better.  But with the advent of
 *  "random access" password files, it might not be worth the effort.
 *
 *  Changes to these have been provided by John Gilmore (gnu@toad.com).
 *
 *  The hash has been simplified in this release, to avoid the
 *  table overflow problems of previous releases.  If the value
 *  at the initial hash location is not right, it is replaced
 *  by the right value.  Collisions will cause us to call getpw*
 *  but hey, this is a cache, not the Library of Congress.
 *  This makes the table size independent of the passwd file size.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "username.h"

struct hash_el {
    int  uid;
    char name[MAXLOGNAME];
};

#define    is_empty_hash(x)	(hash_table[x].name[0] == 0)

/* simple minded hashing function */
/* Uid "nobody" is -2 results in hashit(-2) = -2 which is out of bounds for
   the hash_table.  Applied abs() function to fix. 2/16/96 tpugh
*/
#define    hashit(i)	(abs(i) % Table_size)

/* K&R requires that statically declared tables be initialized to zero. */
/* We depend on that for hash_table and YOUR compiler had BETTER do it! */
static struct hash_el hash_table[Table_size];


char *username(uid)

int uid;

{
    int hashindex;

    hashindex = hashit(uid);
    if (is_empty_hash(hashindex) || (hash_table[hashindex].uid != uid))
    {
	/* not here or not right -- get it out of passwd */
	hashindex = get_user(uid);
    }
    return(hash_table[hashindex].name);
}

int userid(username)

char *username;

{
    struct passwd *pwd;

    /* Eventually we want this to enter everything in the hash table,
       but for now we just do it simply and remember just the result.
     */

    if ((pwd = getpwnam(username)) == NULL)
    {
	return(-1);
    }

    /* enter the result in the hash table */
    enter_user(pwd->pw_uid, username, 1);

    /* return our result */
    return(pwd->pw_uid);
}

int enter_user(uid, name, wecare)

int  uid;
char *name;
int wecare;		/* 1 = enter it always, 0 = nice to have */

{
    int hashindex;

#ifdef DEBUG
    fprintf(stderr, "enter_hash(%d, %s, %d)\n", uid, name, wecare);
#endif

    hashindex = hashit(uid);

    if (!is_empty_hash(hashindex))
    {
	if (!wecare)
	    return 0;		/* Don't clobber a slot for trash */
	if (hash_table[hashindex].uid == uid)
	    return(hashindex);	/* Fortuitous find */
    }

    /* empty or wrong slot -- fill it with new value */
    hash_table[hashindex].uid = uid;
    (void) strncpy(hash_table[hashindex].name, name, MAXLOGNAME - 1);
    return(hashindex);
}

/*
 * Get a userid->name mapping from the system.
 * If the passwd database is hashed (#define RANDOM_PW), we
 * just handle this uid.
 */

int
get_user(int uid)
{
    struct passwd *pwd;

    /* no performance penalty for using getpwuid makes it easy */
    if ((pwd = getpwuid(uid)) != NULL)
    {
	return(enter_user(pwd->pw_uid, pwd->pw_name, 1));
    }

    /* if we can't find the name at all, then use the uid as the name */
    return(enter_user(uid, itoa7(uid), 1));
}
