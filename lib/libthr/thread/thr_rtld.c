/*
 * Copyright (c) 2006, David Xu <davidxu@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

 /*
  * A lockless rwlock for rtld.
  */
#include <sys/cdefs.h>
#include <stdlib.h>

#include "rtld_lock.h"
#include "thr_private.h"

#undef errno
extern int errno;

#define CACHE_LINE_SIZE		64

static int	_thr_rtld_clr_flag(int);
static void	*_thr_rtld_lock_create(void);
static void	_thr_rtld_lock_destroy(void *);
static void	_thr_rtld_lock_release(void *);
static void	_thr_rtld_rlock_acquire(void *);
static int	_thr_rtld_set_flag(int);
static void	_thr_rtld_wlock_acquire(void *);

struct rtld_lock {
	struct	urwlock	lock;
	char		_pad[CACHE_LINE_SIZE - sizeof(struct urwlock)];
};

static struct rtld_lock lock_place[MAX_RTLD_LOCKS] __aligned(CACHE_LINE_SIZE);
static int busy_places;

static void *
_thr_rtld_lock_create(void)
{
	int locki;
	struct rtld_lock *l;
	static const char fail[] = "_thr_rtld_lock_create failed\n";

	for (locki = 0; locki < MAX_RTLD_LOCKS; locki++) {
		if ((busy_places & (1 << locki)) == 0)
			break;
	}
	if (locki == MAX_RTLD_LOCKS) {
		write(2, fail, sizeof(fail) - 1);
		return (NULL);
	}
	busy_places |= (1 << locki);

	l = &lock_place[locki];
	l->lock.rw_flags = URWLOCK_PREFER_READER;
	return (l);
}

static void
_thr_rtld_lock_destroy(void *lock)
{
	int locki;

	locki = (struct rtld_lock *)lock - &lock_place[0];
	busy_places &= ~(1 << locki);
}

#define SAVE_ERRNO()	{			\
	if (curthread != _thr_initial)		\
		errsave = curthread->error;	\
	else					\
		errsave = errno;		\
}

#define RESTORE_ERRNO()	{ 			\
	if (curthread != _thr_initial)  	\
		curthread->error = errsave;	\
	else					\
		errno = errsave;		\
}

static void
_thr_rtld_rlock_acquire(void *lock)
{
	struct pthread		*curthread;
	struct rtld_lock	*l;
	int			errsave;

	curthread = _get_curthread();
	SAVE_ERRNO();
	l = (struct rtld_lock *)lock;

	THR_CRITICAL_ENTER(curthread);
	while (_thr_rwlock_rdlock(&l->lock, 0, NULL) != 0)
		;
	RESTORE_ERRNO();
}

static void
_thr_rtld_wlock_acquire(void *lock)
{
	struct pthread		*curthread;
	struct rtld_lock	*l;
	int			errsave;

	curthread = _get_curthread();
	SAVE_ERRNO();
	l = (struct rtld_lock *)lock;

	_thr_signal_block(curthread);
	while (_thr_rwlock_wrlock(&l->lock, NULL) != 0)
		;
	RESTORE_ERRNO();
}

static void
_thr_rtld_lock_release(void *lock)
{
	struct pthread		*curthread;
	struct rtld_lock	*l;
	int32_t			state;
	int			errsave;

	curthread = _get_curthread();
	SAVE_ERRNO();
	l = (struct rtld_lock *)lock;
	
	state = l->lock.rw_state;
	if (_thr_rwlock_unlock(&l->lock) == 0) {
		if ((state & URWLOCK_WRITE_OWNER) == 0) {
			THR_CRITICAL_LEAVE(curthread);
		} else {
			_thr_signal_unblock(curthread);
		}
	}
	RESTORE_ERRNO();
}

static int
_thr_rtld_set_flag(int mask __unused)
{
	/*
	 * The caller's code in rtld-elf is broken, it is not signal safe,
	 * just return zero to fool it.
	 */
	return (0);
}

static int
_thr_rtld_clr_flag(int mask __unused)
{
	return (0);
}

void
_thr_rtld_init(void)
{
	struct RtldLockInfo	li;
	struct pthread		*curthread;
	long dummy = -1;

	curthread = _get_curthread();

	/* force to resolve _umtx_op PLT */
	_umtx_op_err((struct umtx *)&dummy, UMTX_OP_WAKE, 1, 0, 0);
	
	/* force to resolve errno() PLT */
	__error();

	li.lock_create  = _thr_rtld_lock_create;
	li.lock_destroy = _thr_rtld_lock_destroy;
	li.rlock_acquire = _thr_rtld_rlock_acquire;
	li.wlock_acquire = _thr_rtld_wlock_acquire;
	li.lock_release  = _thr_rtld_lock_release;
	li.thread_set_flag = _thr_rtld_set_flag;
	li.thread_clr_flag = _thr_rtld_clr_flag;
	li.at_fork = NULL;
	
	/* mask signals, also force to resolve __sys_sigprocmask PLT */
	_thr_signal_block(curthread);
	_rtld_thread_init(&li);
	_thr_signal_unblock(curthread);
}

void
_thr_rtld_fini(void)
{
	struct pthread	*curthread;

	curthread = _get_curthread();
	_thr_signal_block(curthread);
	_rtld_thread_init(NULL);
	_thr_signal_unblock(curthread);
}
