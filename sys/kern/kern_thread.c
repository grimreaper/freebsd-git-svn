/*-
 * Copyright (C) 2001 Julian Elischer <julian@freebsd.org>.
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "opt_witness.h"
#include "opt_hwpmc_hooks.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/smp.h>
#include <sys/sysctl.h>
#include <sys/sched.h>
#include <sys/sleepqueue.h>
#include <sys/selinfo.h>
#include <sys/turnstile.h>
#include <sys/ktr.h>
#include <sys/umtx.h>
#include <sys/cpuset.h>
#ifdef	HWPMC_HOOKS
#include <sys/pmckern.h>
#endif

#include <security/audit/audit.h>

#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/uma.h>
#include <sys/eventhandler.h>

/*
 * thread related storage.
 */
static uma_zone_t thread_zone;

SYSCTL_NODE(_kern, OID_AUTO, threads, CTLFLAG_RW, 0, "thread allocation");

int max_threads_per_proc = 1500;
SYSCTL_INT(_kern_threads, OID_AUTO, max_threads_per_proc, CTLFLAG_RW,
	&max_threads_per_proc, 0, "Limit on threads per proc");

int max_threads_hits;
SYSCTL_INT(_kern_threads, OID_AUTO, max_threads_hits, CTLFLAG_RD,
	&max_threads_hits, 0, "");

TAILQ_HEAD(, thread) zombie_threads = TAILQ_HEAD_INITIALIZER(zombie_threads);
static struct mtx zombie_lock;
MTX_SYSINIT(zombie_lock, &zombie_lock, "zombie lock", MTX_SPIN);

static void thread_zombie(struct thread *);

struct mtx tid_lock;
static struct unrhdr *tid_unrhdr;

/*
 * Prepare a thread for use.
 */
static int
thread_ctor(void *mem, int size, void *arg, int flags)
{
	struct thread	*td;

	td = (struct thread *)mem;
	td->td_state = TDS_INACTIVE;
	td->td_oncpu = NOCPU;

	td->td_tid = alloc_unr(tid_unrhdr);
	td->td_syscalls = 0;

	/*
	 * Note that td_critnest begins life as 1 because the thread is not
	 * running and is thereby implicitly waiting to be on the receiving
	 * end of a context switch.
	 */
	td->td_critnest = 1;
	EVENTHANDLER_INVOKE(thread_ctor, td);
#ifdef AUDIT
	audit_thread_alloc(td);
#endif
	umtx_thread_alloc(td);
	return (0);
}

/*
 * Reclaim a thread after use.
 */
static void
thread_dtor(void *mem, int size, void *arg)
{
	struct thread *td;

	td = (struct thread *)mem;

#ifdef INVARIANTS
	/* Verify that this thread is in a safe state to free. */
	switch (td->td_state) {
	case TDS_INHIBITED:
	case TDS_RUNNING:
	case TDS_CAN_RUN:
	case TDS_RUNQ:
		/*
		 * We must never unlink a thread that is in one of
		 * these states, because it is currently active.
		 */
		panic("bad state for thread unlinking");
		/* NOTREACHED */
	case TDS_INACTIVE:
		break;
	default:
		panic("bad thread state");
		/* NOTREACHED */
	}
#endif
#ifdef AUDIT
	audit_thread_free(td);
#endif
	/* Free all OSD associated to this thread. */
	osd_thread_exit(td);

	EVENTHANDLER_INVOKE(thread_dtor, td);
	free_unr(tid_unrhdr, td->td_tid);
}

/*
 * Initialize type-stable parts of a thread (when newly created).
 */
static int
thread_init(void *mem, int size, int flags)
{
	struct thread *td;

	td = (struct thread *)mem;

	td->td_sleepqueue = sleepq_alloc();
	td->td_turnstile = turnstile_alloc();
	EVENTHANDLER_INVOKE(thread_init, td);
	td->td_sched = (struct td_sched *)&td[1];
	umtx_thread_init(td);
	td->td_kstack = 0;
	return (0);
}

/*
 * Tear down type-stable parts of a thread (just before being discarded).
 */
static void
thread_fini(void *mem, int size)
{
	struct thread *td;

	td = (struct thread *)mem;
	EVENTHANDLER_INVOKE(thread_fini, td);
	turnstile_free(td->td_turnstile);
	sleepq_free(td->td_sleepqueue);
	umtx_thread_fini(td);
	seltdfini(td);
}

/*
 * For a newly created process,
 * link up all the structures and its initial threads etc.
 * called from:
 * {arch}/{arch}/machdep.c   ia64_init(), init386() etc.
 * proc_dtor() (should go away)
 * proc_init()
 */
void
proc_linkup0(struct proc *p, struct thread *td)
{
	TAILQ_INIT(&p->p_threads);	     /* all threads in proc */
	proc_linkup(p, td);
}

void
proc_linkup(struct proc *p, struct thread *td)
{

	sigqueue_init(&p->p_sigqueue, p);
	p->p_ksi = ksiginfo_alloc(1);
	if (p->p_ksi != NULL) {
		/* XXX p_ksi may be null if ksiginfo zone is not ready */
		p->p_ksi->ksi_flags = KSI_EXT | KSI_INS;
	}
	LIST_INIT(&p->p_mqnotifier);
	p->p_numthreads = 0;
	thread_link(td, p);
}

/*
 * Initialize global thread allocation resources.
 */
void
threadinit(void)
{

	mtx_init(&tid_lock, "TID lock", NULL, MTX_DEF);
	/* leave one number for thread0 */
	tid_unrhdr = new_unrhdr(PID_MAX + 2, INT_MAX, &tid_lock);

	thread_zone = uma_zcreate("THREAD", sched_sizeof_thread(),
	    thread_ctor, thread_dtor, thread_init, thread_fini,
	    16 - 1, 0);
}

/*
 * Place an unused thread on the zombie list.
 * Use the slpq as that must be unused by now.
 */
void
thread_zombie(struct thread *td)
{
	mtx_lock_spin(&zombie_lock);
	TAILQ_INSERT_HEAD(&zombie_threads, td, td_slpq);
	mtx_unlock_spin(&zombie_lock);
}

/*
 * Release a thread that has exited after cpu_throw().
 */
void
thread_stash(struct thread *td)
{
	atomic_subtract_rel_int(&td->td_proc->p_exitthreads, 1);
	thread_zombie(td);
}

/*
 * Reap zombie resources.
 */
void
thread_reap(void)
{
	struct thread *td_first, *td_next;

	/*
	 * Don't even bother to lock if none at this instant,
	 * we really don't care about the next instant..
	 */
	if (!TAILQ_EMPTY(&zombie_threads)) {
		mtx_lock_spin(&zombie_lock);
		td_first = TAILQ_FIRST(&zombie_threads);
		if (td_first)
			TAILQ_INIT(&zombie_threads);
		mtx_unlock_spin(&zombie_lock);
		while (td_first) {
			td_next = TAILQ_NEXT(td_first, td_slpq);
			if (td_first->td_ucred)
				crfree(td_first->td_ucred);
			thread_free(td_first);
			td_first = td_next;
		}
	}
}

/*
 * Allocate a thread.
 */
struct thread *
thread_alloc(int pages)
{
	struct thread *td;

	thread_reap(); /* check if any zombies to get */

	td = (struct thread *)uma_zalloc(thread_zone, M_WAITOK);
	KASSERT(td->td_kstack == 0, ("thread_alloc got thread with kstack"));
	if (!vm_thread_new(td, pages)) {
		uma_zfree(thread_zone, td);
		return (NULL);
	}
	cpu_thread_alloc(td);
	return (td);
}

int
thread_alloc_stack(struct thread *td, int pages)
{

	KASSERT(td->td_kstack == 0,
	    ("thread_alloc_stack called on a thread with kstack"));
	if (!vm_thread_new(td, pages))
		return (0);
	cpu_thread_alloc(td);
	return (1);
}

/*
 * Deallocate a thread.
 */
void
thread_free(struct thread *td)
{

	lock_profile_thread_exit(td);
	if (td->td_cpuset)
		cpuset_rel(td->td_cpuset);
	td->td_cpuset = NULL;
	cpu_thread_free(td);
	if (td->td_kstack != 0)
		vm_thread_dispose(td);
	uma_zfree(thread_zone, td);
}

/*
 * Discard the current thread and exit from its context.
 * Always called with scheduler locked.
 *
 * Because we can't free a thread while we're operating under its context,
 * push the current thread into our CPU's deadthread holder. This means
 * we needn't worry about someone else grabbing our context before we
 * do a cpu_throw().
 */
void
thread_exit(void)
{
	uint64_t new_switchtime;
	struct thread *td;
	struct thread *td2;
	struct proc *p;
	int wakeup_swapper;

	td = curthread;
	p = td->td_proc;

	PROC_SLOCK_ASSERT(p, MA_OWNED);
	mtx_assert(&Giant, MA_NOTOWNED);

	PROC_LOCK_ASSERT(p, MA_OWNED);
	KASSERT(p != NULL, ("thread exiting without a process"));
	CTR3(KTR_PROC, "thread_exit: thread %p (pid %ld, %s)", td,
	    (long)p->p_pid, td->td_name);
	KASSERT(TAILQ_EMPTY(&td->td_sigqueue.sq_list), ("signal pending"));

#ifdef AUDIT
	AUDIT_SYSCALL_EXIT(0, td);
#endif
	umtx_thread_exit(td);
	/*
	 * drop FPU & debug register state storage, or any other
	 * architecture specific resources that
	 * would not be on a new untouched process.
	 */
	cpu_thread_exit(td);	/* XXXSMP */

	/* Do the same timestamp bookkeeping that mi_switch() would do. */
	new_switchtime = cpu_ticks();
	p->p_rux.rux_runtime += (new_switchtime - PCPU_GET(switchtime));
	PCPU_SET(switchtime, new_switchtime);
	PCPU_SET(switchticks, ticks);
	PCPU_INC(cnt.v_swtch);
	/* Save our resource usage in our process. */
	td->td_ru.ru_nvcsw++;
	rucollect(&p->p_ru, &td->td_ru);
	/*
	 * The last thread is left attached to the process
	 * So that the whole bundle gets recycled. Skip
	 * all this stuff if we never had threads.
	 * EXIT clears all sign of other threads when
	 * it goes to single threading, so the last thread always
	 * takes the short path.
	 */
	if (p->p_flag & P_HADTHREADS) {
		if (p->p_numthreads > 1) {
			thread_unlink(td);
			td2 = FIRST_THREAD_IN_PROC(p);
			sched_exit_thread(td2, td);

			/*
			 * The test below is NOT true if we are the
			 * sole exiting thread. P_STOPPED_SINGLE is unset
			 * in exit1() after it is the only survivor.
			 */
			if (P_SHOULDSTOP(p) == P_STOPPED_SINGLE) {
				if (p->p_numthreads == p->p_suspcount) {
					thread_lock(p->p_singlethread);
					wakeup_swapper = thread_unsuspend_one(
						p->p_singlethread);
					thread_unlock(p->p_singlethread);
					if (wakeup_swapper)
						kick_proc0();
				}
			}

			atomic_add_int(&td->td_proc->p_exitthreads, 1);
			PCPU_SET(deadthread, td);
		} else {
			/*
			 * The last thread is exiting.. but not through exit()
			 */
			panic ("thread_exit: Last thread exiting on its own");
		}
	} 
#ifdef	HWPMC_HOOKS
	/*
	 * If this thread is part of a process that is being tracked by hwpmc(4),
	 * inform the module of the thread's impending exit.
	 */
	if (PMC_PROC_IS_USING_PMCS(td->td_proc))
		PMC_SWITCH_CONTEXT(td, PMC_FN_CSW_OUT);
#endif
	PROC_UNLOCK(p);
	thread_lock(td);
	ruxagg_locked(&p->p_rux, td);
	PROC_SUNLOCK(p);
	td->td_state = TDS_INACTIVE;
#ifdef WITNESS
	witness_thread_exit(td);
#endif
	CTR1(KTR_PROC, "thread_exit: cpu_throw() thread %p", td);
	sched_throw(td);
	panic("I'm a teapot!");
	/* NOTREACHED */
}

/*
 * Do any thread specific cleanups that may be needed in wait()
 * called with Giant, proc and schedlock not held.
 */
void
thread_wait(struct proc *p)
{
	struct thread *td;

	mtx_assert(&Giant, MA_NOTOWNED);
	KASSERT((p->p_numthreads == 1), ("Multiple threads in wait1()"));
	td = FIRST_THREAD_IN_PROC(p);
	/* Lock the last thread so we spin until it exits cpu_throw(). */
	thread_lock(td);
	thread_unlock(td);
	/* Wait for any remaining threads to exit cpu_throw(). */
	while (p->p_exitthreads)
		sched_relinquish(curthread);
	lock_profile_thread_exit(td);
	cpuset_rel(td->td_cpuset);
	td->td_cpuset = NULL;
	cpu_thread_clean(td);
	crfree(td->td_ucred);
	thread_reap();	/* check for zombie threads etc. */
}

/*
 * Link a thread to a process.
 * set up anything that needs to be initialized for it to
 * be used by the process.
 */
void
thread_link(struct thread *td, struct proc *p)
{

	/*
	 * XXX This can't be enabled because it's called for proc0 before
	 * its lock has been created.
	 * PROC_LOCK_ASSERT(p, MA_OWNED);
	 */
	td->td_state    = TDS_INACTIVE;
	td->td_proc     = p;
	td->td_flags    = TDF_INMEM;

	LIST_INIT(&td->td_contested);
	LIST_INIT(&td->td_lprof[0]);
	LIST_INIT(&td->td_lprof[1]);
	sigqueue_init(&td->td_sigqueue, p);
	callout_init(&td->td_slpcallout, CALLOUT_MPSAFE);
	TAILQ_INSERT_HEAD(&p->p_threads, td, td_plist);
	p->p_numthreads++;
}

/*
 * Convert a process with one thread to an unthreaded process.
 */
void
thread_unthread(struct thread *td)
{
	struct proc *p = td->td_proc;

	KASSERT((p->p_numthreads == 1), ("Unthreading with >1 threads"));
	p->p_flag &= ~P_HADTHREADS;
}

/*
 * Called from:
 *  thread_exit()
 */
void
thread_unlink(struct thread *td)
{
	struct proc *p = td->td_proc;

	PROC_LOCK_ASSERT(p, MA_OWNED);
	TAILQ_REMOVE(&p->p_threads, td, td_plist);
	p->p_numthreads--;
	/* could clear a few other things here */
	/* Must  NOT clear links to proc! */
}

static int
calc_remaining(struct proc *p, int mode)
{
	int remaining;

	if (mode == SINGLE_EXIT)
		remaining = p->p_numthreads;
	else if (mode == SINGLE_BOUNDARY)
		remaining = p->p_numthreads - p->p_boundary_count;
	else if (mode == SINGLE_NO_EXIT)
		remaining = p->p_numthreads - p->p_suspcount;
	else
		panic("calc_remaining: wrong mode %d", mode);
	return (remaining);
}

/*
 * Enforce single-threading.
 *
 * Returns 1 if the caller must abort (another thread is waiting to
 * exit the process or similar). Process is locked!
 * Returns 0 when you are successfully the only thread running.
 * A process has successfully single threaded in the suspend mode when
 * There are no threads in user mode. Threads in the kernel must be
 * allowed to continue until they get to the user boundary. They may even
 * copy out their return values and data before suspending. They may however be
 * accelerated in reaching the user boundary as we will wake up
 * any sleeping threads that are interruptable. (PCATCH).
 */
int
thread_single(int mode)
{
	struct thread *td;
	struct thread *td2;
	struct proc *p;
	int remaining, wakeup_swapper;

	td = curthread;
	p = td->td_proc;
	mtx_assert(&Giant, MA_NOTOWNED);
	PROC_LOCK_ASSERT(p, MA_OWNED);
	KASSERT((td != NULL), ("curthread is NULL"));

	if ((p->p_flag & P_HADTHREADS) == 0)
		return (0);

	/* Is someone already single threading? */
	if (p->p_singlethread != NULL && p->p_singlethread != td)
		return (1);

	if (mode == SINGLE_EXIT) {
		p->p_flag |= P_SINGLE_EXIT;
		p->p_flag &= ~P_SINGLE_BOUNDARY;
	} else {
		p->p_flag &= ~P_SINGLE_EXIT;
		if (mode == SINGLE_BOUNDARY)
			p->p_flag |= P_SINGLE_BOUNDARY;
		else
			p->p_flag &= ~P_SINGLE_BOUNDARY;
	}
	p->p_flag |= P_STOPPED_SINGLE;
	PROC_SLOCK(p);
	p->p_singlethread = td;
	remaining = calc_remaining(p, mode);
	while (remaining != 1) {
		if (P_SHOULDSTOP(p) != P_STOPPED_SINGLE)
			goto stopme;
		wakeup_swapper = 0;
		FOREACH_THREAD_IN_PROC(p, td2) {
			if (td2 == td)
				continue;
			thread_lock(td2);
			td2->td_flags |= TDF_ASTPENDING | TDF_NEEDSUSPCHK;
			if (TD_IS_INHIBITED(td2)) {
				switch (mode) {
				case SINGLE_EXIT:
					if (TD_IS_SUSPENDED(td2))
						wakeup_swapper |=
						    thread_unsuspend_one(td2);
					if (TD_ON_SLEEPQ(td2) &&
					    (td2->td_flags & TDF_SINTR))
						wakeup_swapper |=
						    sleepq_abort(td2, EINTR);
					break;
				case SINGLE_BOUNDARY:
					if (TD_IS_SUSPENDED(td2) &&
					    !(td2->td_flags & TDF_BOUNDARY))
						wakeup_swapper |=
						    thread_unsuspend_one(td2);
					if (TD_ON_SLEEPQ(td2) &&
					    (td2->td_flags & TDF_SINTR))
						wakeup_swapper |=
						    sleepq_abort(td2, ERESTART);
					break;
				case SINGLE_NO_EXIT:
					if (TD_IS_SUSPENDED(td2) &&
					    !(td2->td_flags & TDF_BOUNDARY))
						wakeup_swapper |=
						    thread_unsuspend_one(td2);
					if (TD_ON_SLEEPQ(td2) &&
					    (td2->td_flags & TDF_SINTR))
						wakeup_swapper |=
						    sleepq_abort(td2, ERESTART);
					break;
				default:
					break;
				}
			}
#ifdef SMP
			else if (TD_IS_RUNNING(td2) && td != td2) {
				forward_signal(td2);
			}
#endif
			thread_unlock(td2);
		}
		if (wakeup_swapper)
			kick_proc0();
		remaining = calc_remaining(p, mode);

		/*
		 * Maybe we suspended some threads.. was it enough?
		 */
		if (remaining == 1)
			break;

stopme:
		/*
		 * Wake us up when everyone else has suspended.
		 * In the mean time we suspend as well.
		 */
		thread_suspend_switch(td);
		remaining = calc_remaining(p, mode);
	}
	if (mode == SINGLE_EXIT) {
		/*
		 * We have gotten rid of all the other threads and we
		 * are about to either exit or exec. In either case,
		 * we try our utmost  to revert to being a non-threaded
		 * process.
		 */
		p->p_singlethread = NULL;
		p->p_flag &= ~(P_STOPPED_SINGLE | P_SINGLE_EXIT);
		thread_unthread(td);
	}
	PROC_SUNLOCK(p);
	return (0);
}

/*
 * Called in from locations that can safely check to see
 * whether we have to suspend or at least throttle for a
 * single-thread event (e.g. fork).
 *
 * Such locations include userret().
 * If the "return_instead" argument is non zero, the thread must be able to
 * accept 0 (caller may continue), or 1 (caller must abort) as a result.
 *
 * The 'return_instead' argument tells the function if it may do a
 * thread_exit() or suspend, or whether the caller must abort and back
 * out instead.
 *
 * If the thread that set the single_threading request has set the
 * P_SINGLE_EXIT bit in the process flags then this call will never return
 * if 'return_instead' is false, but will exit.
 *
 * P_SINGLE_EXIT | return_instead == 0| return_instead != 0
 *---------------+--------------------+---------------------
 *       0       | returns 0          |   returns 0 or 1
 *               | when ST ends       |   immediatly
 *---------------+--------------------+---------------------
 *       1       | thread exits       |   returns 1
 *               |                    |  immediatly
 * 0 = thread_exit() or suspension ok,
 * other = return error instead of stopping the thread.
 *
 * While a full suspension is under effect, even a single threading
 * thread would be suspended if it made this call (but it shouldn't).
 * This call should only be made from places where
 * thread_exit() would be safe as that may be the outcome unless
 * return_instead is set.
 */
int
thread_suspend_check(int return_instead)
{
	struct thread *td;
	struct proc *p;
	int wakeup_swapper;

	td = curthread;
	p = td->td_proc;
	mtx_assert(&Giant, MA_NOTOWNED);
	PROC_LOCK_ASSERT(p, MA_OWNED);
	while (P_SHOULDSTOP(p) ||
	      ((p->p_flag & P_TRACED) && (td->td_dbgflags & TDB_SUSPEND))) {
		if (P_SHOULDSTOP(p) == P_STOPPED_SINGLE) {
			KASSERT(p->p_singlethread != NULL,
			    ("singlethread not set"));
			/*
			 * The only suspension in action is a
			 * single-threading. Single threader need not stop.
			 * XXX Should be safe to access unlocked
			 * as it can only be set to be true by us.
			 */
			if (p->p_singlethread == td)
				return (0);	/* Exempt from stopping. */
		}
		if ((p->p_flag & P_SINGLE_EXIT) && return_instead)
			return (EINTR);

		/* Should we goto user boundary if we didn't come from there? */
		if (P_SHOULDSTOP(p) == P_STOPPED_SINGLE &&
		    (p->p_flag & P_SINGLE_BOUNDARY) && return_instead)
			return (ERESTART);

		/* If thread will exit, flush its pending signals */
		if ((p->p_flag & P_SINGLE_EXIT) && (p->p_singlethread != td))
			sigqueue_flush(&td->td_sigqueue);

		PROC_SLOCK(p);
		thread_stopped(p);
		/*
		 * If the process is waiting for us to exit,
		 * this thread should just suicide.
		 * Assumes that P_SINGLE_EXIT implies P_STOPPED_SINGLE.
		 */
		if ((p->p_flag & P_SINGLE_EXIT) && (p->p_singlethread != td))
			thread_exit();
		if (P_SHOULDSTOP(p) == P_STOPPED_SINGLE) {
			if (p->p_numthreads == p->p_suspcount + 1) {
				thread_lock(p->p_singlethread);
				wakeup_swapper =
				    thread_unsuspend_one(p->p_singlethread);
				thread_unlock(p->p_singlethread);
				if (wakeup_swapper)
					kick_proc0();
			}
		}
		PROC_UNLOCK(p);
		thread_lock(td);
		/*
		 * When a thread suspends, it just
		 * gets taken off all queues.
		 */
		thread_suspend_one(td);
		if (return_instead == 0) {
			p->p_boundary_count++;
			td->td_flags |= TDF_BOUNDARY;
		}
		PROC_SUNLOCK(p);
		mi_switch(SW_INVOL | SWT_SUSPEND, NULL);
		if (return_instead == 0)
			td->td_flags &= ~TDF_BOUNDARY;
		thread_unlock(td);
		PROC_LOCK(p);
		if (return_instead == 0)
			p->p_boundary_count--;
	}
	return (0);
}

void
thread_suspend_switch(struct thread *td)
{
	struct proc *p;

	p = td->td_proc;
	KASSERT(!TD_IS_SUSPENDED(td), ("already suspended"));
	PROC_LOCK_ASSERT(p, MA_OWNED);
	PROC_SLOCK_ASSERT(p, MA_OWNED);
	/*
	 * We implement thread_suspend_one in stages here to avoid
	 * dropping the proc lock while the thread lock is owned.
	 */
	thread_stopped(p);
	p->p_suspcount++;
	PROC_UNLOCK(p);
	thread_lock(td);
	td->td_flags &= ~TDF_NEEDSUSPCHK;
	TD_SET_SUSPENDED(td);
	sched_sleep(td, 0);
	PROC_SUNLOCK(p);
	DROP_GIANT();
	mi_switch(SW_VOL | SWT_SUSPEND, NULL);
	thread_unlock(td);
	PICKUP_GIANT();
	PROC_LOCK(p);
	PROC_SLOCK(p);
}

void
thread_suspend_one(struct thread *td)
{
	struct proc *p = td->td_proc;

	PROC_SLOCK_ASSERT(p, MA_OWNED);
	THREAD_LOCK_ASSERT(td, MA_OWNED);
	KASSERT(!TD_IS_SUSPENDED(td), ("already suspended"));
	p->p_suspcount++;
	td->td_flags &= ~TDF_NEEDSUSPCHK;
	TD_SET_SUSPENDED(td);
	sched_sleep(td, 0);
}

int
thread_unsuspend_one(struct thread *td)
{
	struct proc *p = td->td_proc;

	PROC_SLOCK_ASSERT(p, MA_OWNED);
	THREAD_LOCK_ASSERT(td, MA_OWNED);
	KASSERT(TD_IS_SUSPENDED(td), ("Thread not suspended"));
	TD_CLR_SUSPENDED(td);
	p->p_suspcount--;
	return (setrunnable(td));
}

/*
 * Allow all threads blocked by single threading to continue running.
 */
void
thread_unsuspend(struct proc *p)
{
	struct thread *td;
	int wakeup_swapper;

	PROC_LOCK_ASSERT(p, MA_OWNED);
	PROC_SLOCK_ASSERT(p, MA_OWNED);
	wakeup_swapper = 0;
	if (!P_SHOULDSTOP(p)) {
                FOREACH_THREAD_IN_PROC(p, td) {
			thread_lock(td);
			if (TD_IS_SUSPENDED(td)) {
				wakeup_swapper |= thread_unsuspend_one(td);
			}
			thread_unlock(td);
		}
	} else if ((P_SHOULDSTOP(p) == P_STOPPED_SINGLE) &&
	    (p->p_numthreads == p->p_suspcount)) {
		/*
		 * Stopping everything also did the job for the single
		 * threading request. Now we've downgraded to single-threaded,
		 * let it continue.
		 */
		thread_lock(p->p_singlethread);
		wakeup_swapper = thread_unsuspend_one(p->p_singlethread);
		thread_unlock(p->p_singlethread);
	}
	if (wakeup_swapper)
		kick_proc0();
}

/*
 * End the single threading mode..
 */
void
thread_single_end(void)
{
	struct thread *td;
	struct proc *p;
	int wakeup_swapper;

	td = curthread;
	p = td->td_proc;
	PROC_LOCK_ASSERT(p, MA_OWNED);
	p->p_flag &= ~(P_STOPPED_SINGLE | P_SINGLE_EXIT | P_SINGLE_BOUNDARY);
	PROC_SLOCK(p);
	p->p_singlethread = NULL;
	wakeup_swapper = 0;
	/*
	 * If there are other threads they may now run,
	 * unless of course there is a blanket 'stop order'
	 * on the process. The single threader must be allowed
	 * to continue however as this is a bad place to stop.
	 */
	if ((p->p_numthreads != 1) && (!P_SHOULDSTOP(p))) {
                FOREACH_THREAD_IN_PROC(p, td) {
			thread_lock(td);
			if (TD_IS_SUSPENDED(td)) {
				wakeup_swapper |= thread_unsuspend_one(td);
			}
			thread_unlock(td);
		}
	}
	PROC_SUNLOCK(p);
	if (wakeup_swapper)
		kick_proc0();
}

struct thread *
thread_find(struct proc *p, lwpid_t tid)
{
	struct thread *td;

	PROC_LOCK_ASSERT(p, MA_OWNED);
	FOREACH_THREAD_IN_PROC(p, td) {
		if (td->td_tid == tid)
			break;
	}
	return (td);
}
