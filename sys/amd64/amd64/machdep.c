/*-
 * Copyright (c) 1992 Terrence R. Lambert.
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)machdep.c	7.4 (Berkeley) 6/3/91
 * $FreeBSD$
 */

#include "apm.h"
#include "ether.h"
#include "npx.h"
#include "opt_atalk.h"
#include "opt_cpu.h"
#include "opt_ddb.h"
#include "opt_inet.h"
#include "opt_ipx.h"
#include "opt_maxmem.h"
#include "opt_msgbuf.h"
#include "opt_perfmon.h"
#include "opt_smp.h"
#include "opt_sysvipc.h"
#include "opt_user_ldt.h"
#include "opt_userconfig.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/linker.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/sysent.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <sys/bus.h>

#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#ifdef SYSVMSG
#include <sys/msg.h>
#endif

#ifdef SYSVSEM
#include <sys/sem.h>
#endif

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_prot.h>
#include <sys/lock.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_pager.h>
#include <vm/vm_extern.h>

#include <sys/user.h>
#include <sys/exec.h>
#include <sys/cons.h>

#include <ddb/ddb.h>

#include <net/netisr.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/clock.h>
#include <machine/specialreg.h>
#include <machine/bootinfo.h>
#include <machine/ipl.h>
#include <machine/md_var.h>
#include <machine/pcb_ext.h>		/* pcb.h included via sys/user.h */
#ifdef SMP
#include <machine/smp.h>
#include <machine/globaldata.h>
#endif
#ifdef PERFMON
#include <machine/perfmon.h>
#endif

#ifdef OLD_BUS_ARCH
#include <i386/isa/isa_device.h>
#endif
#include <i386/isa/intr_machdep.h>
#include <isa/rtc.h>
#include <machine/vm86.h>
#include <machine/random.h>
#include <sys/ptrace.h>

extern void init386 __P((int first));
extern void dblfault_handler __P((void));

extern void printcpuinfo(void);	/* XXX header file */
extern void earlysetcpuclass(void);	/* same header file */
extern void finishidentcpu(void);
extern void panicifcpuunsupported(void);
extern void initializecpu(void);

static void cpu_startup __P((void *));
SYSINIT(cpu, SI_SUB_CPU, SI_ORDER_FIRST, cpu_startup, NULL)

static MALLOC_DEFINE(M_MBUF, "mbuf", "mbuf");

int	_udatasel, _ucodesel;
u_int	atdevbase;

#if defined(SWTCH_OPTIM_STATS)
extern int swtch_optim_stats;
SYSCTL_INT(_debug, OID_AUTO, swtch_optim_stats,
	CTLFLAG_RD, &swtch_optim_stats, 0, "");
SYSCTL_INT(_debug, OID_AUTO, tlb_flush_count,
	CTLFLAG_RD, &tlb_flush_count, 0, "");
#endif

#ifdef PC98
static int	ispc98 = 1;
#else
static int	ispc98 = 0;
#endif
SYSCTL_INT(_machdep, OID_AUTO, ispc98, CTLFLAG_RD, &ispc98, 0, "");

int physmem = 0;
int cold = 1;

static int
sysctl_hw_physmem SYSCTL_HANDLER_ARGS
{
	int error = sysctl_handle_int(oidp, 0, ctob(physmem), req);
	return (error);
}

SYSCTL_PROC(_hw, HW_PHYSMEM, physmem, CTLTYPE_INT|CTLFLAG_RD,
	0, 0, sysctl_hw_physmem, "I", "");

static int
sysctl_hw_usermem SYSCTL_HANDLER_ARGS
{
	int error = sysctl_handle_int(oidp, 0,
		ctob(physmem - cnt.v_wire_count), req);
	return (error);
}

SYSCTL_PROC(_hw, HW_USERMEM, usermem, CTLTYPE_INT|CTLFLAG_RD,
	0, 0, sysctl_hw_usermem, "I", "");

static int
sysctl_hw_availpages SYSCTL_HANDLER_ARGS
{
	int error = sysctl_handle_int(oidp, 0,
		i386_btop(avail_end - avail_start), req);
	return (error);
}

SYSCTL_PROC(_hw, OID_AUTO, availpages, CTLTYPE_INT|CTLFLAG_RD,
	0, 0, sysctl_hw_availpages, "I", "");

static int
sysctl_machdep_msgbuf SYSCTL_HANDLER_ARGS
{
	int error;

	/* Unwind the buffer, so that it's linear (possibly starting with
	 * some initial nulls).
	 */
	error=sysctl_handle_opaque(oidp,msgbufp->msg_ptr+msgbufp->msg_bufr,
		msgbufp->msg_size-msgbufp->msg_bufr,req);
	if(error) return(error);
	if(msgbufp->msg_bufr>0) {
		error=sysctl_handle_opaque(oidp,msgbufp->msg_ptr,
			msgbufp->msg_bufr,req);
	}
	return(error);
}

SYSCTL_PROC(_machdep, OID_AUTO, msgbuf, CTLTYPE_STRING|CTLFLAG_RD,
	0, 0, sysctl_machdep_msgbuf, "A","Contents of kernel message buffer");

static int msgbuf_clear;

static int
sysctl_machdep_msgbuf_clear SYSCTL_HANDLER_ARGS
{
	int error;
	error = sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2,
		req);
	if (!error && req->newptr) {
		/* Clear the buffer and reset write pointer */
		bzero(msgbufp->msg_ptr,msgbufp->msg_size);
		msgbufp->msg_bufr=msgbufp->msg_bufx=0;
		msgbuf_clear=0;
	}
	return (error);
}

SYSCTL_PROC(_machdep, OID_AUTO, msgbuf_clear, CTLTYPE_INT|CTLFLAG_RW,
	&msgbuf_clear, 0, sysctl_machdep_msgbuf_clear, "I",
	"Clear kernel message buffer");

int bootverbose = 0, Maxmem = 0;
long dumplo;

vm_offset_t phys_avail[10];

/* must be 2 less so 0 0 can signal end of chunks */
#define PHYS_AVAIL_ARRAY_END ((sizeof(phys_avail) / sizeof(vm_offset_t)) - 2)

static vm_offset_t buffer_sva, buffer_eva;
vm_offset_t clean_sva, clean_eva;
static vm_offset_t pager_sva, pager_eva;

#define offsetof(type, member)	((size_t)(&((type *)0)->member))

static void
cpu_startup(dummy)
	void *dummy;
{
	register unsigned i;
	register caddr_t v;
	vm_offset_t maxaddr;
	vm_size_t size = 0;
	int firstaddr;
	vm_offset_t minaddr;

	if (boothowto & RB_VERBOSE)
		bootverbose++;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	earlysetcpuclass();
	startrtclock();
	printcpuinfo();
	panicifcpuunsupported();
#ifdef PERFMON
	perfmon_init();
#endif
	printf("real memory  = %u (%uK bytes)\n", ptoa(Maxmem), ptoa(Maxmem) / 1024);
	/*
	 * Display any holes after the first chunk of extended memory.
	 */
	if (bootverbose) {
		int indx;

		printf("Physical memory chunk(s):\n");
		for (indx = 0; phys_avail[indx + 1] != 0; indx += 2) {
			int size1 = phys_avail[indx + 1] - phys_avail[indx];

			printf("0x%08x - 0x%08x, %u bytes (%u pages)\n",
			    phys_avail[indx], phys_avail[indx + 1] - 1, size1,
			    size1 / PAGE_SIZE);
		}
	}

	/*
	 * Calculate callout wheel size
	 */
	for (callwheelsize = 1, callwheelbits = 0;
	     callwheelsize < ncallout;
	     callwheelsize <<= 1, ++callwheelbits)
		;
	callwheelmask = callwheelsize - 1;

	/*
	 * Allocate space for system data structures.
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */

	/*
	 * Make two passes.  The first pass calculates how much memory is
	 * needed and allocates it.  The second pass assigns virtual
	 * addresses to the various data structures.
	 */
	firstaddr = 0;
again:
	v = (caddr_t)firstaddr;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))

	valloc(callout, struct callout, ncallout);
	valloc(callwheel, struct callout_tailq, callwheelsize);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

	if (nbuf == 0) {
		nbuf = 30;
		if (physmem > 1024)
			nbuf += min((physmem - 1024) / 8, 2048);
		if (physmem > 16384)
			nbuf += (physmem - 16384) / 20;
	}
	nswbuf = max(min(nbuf/4, 256), 16);

	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	v = bufhashinit(v);

	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if (firstaddr == 0) {
		size = (vm_size_t)(v - firstaddr);
		firstaddr = (int)kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}

	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
		panic("startup: table size inconsistency");

	clean_map = kmem_suballoc(kernel_map, &clean_sva, &clean_eva,
			(nbuf*BKVASIZE) + (nswbuf*MAXPHYS) + pager_map_size);
	buffer_map = kmem_suballoc(clean_map, &buffer_sva, &buffer_eva,
				(nbuf*BKVASIZE));
	pager_map = kmem_suballoc(clean_map, &pager_sva, &pager_eva,
				(nswbuf*MAXPHYS) + pager_map_size);
	pager_map->system_map = 1;
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				(16*(ARG_MAX+(PAGE_SIZE*3))));

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	{
		vm_offset_t mb_map_size;

		mb_map_size = nmbufs * MSIZE + nmbclusters * MCLBYTES;
		mb_map_size = roundup2(mb_map_size, max(MCLBYTES, PAGE_SIZE));
		mclrefcnt = malloc(mb_map_size / MCLBYTES, M_MBUF, M_NOWAIT);
		bzero(mclrefcnt, mb_map_size / MCLBYTES);
		mb_map = kmem_suballoc(kmem_map, (vm_offset_t *)&mbutl, &maxaddr,
			mb_map_size);
		mb_map->system_map = 1;
	}

	/*
	 * Initialize callouts
	 */
	SLIST_INIT(&callfree);
	for (i = 0; i < ncallout; i++) {
		callout_init(&callout[i]);
		callout[i].c_flags = CALLOUT_LOCAL_ALLOC;
		SLIST_INSERT_HEAD(&callfree, &callout[i], c_links.sle);
	}

	for (i = 0; i < callwheelsize; i++) {
		TAILQ_INIT(&callwheel[i]);
	}

#if defined(USERCONFIG)
	userconfig();
	cninit();		/* the preferred console may have changed */
#endif

	printf("avail memory = %u (%uK bytes)\n", ptoa(cnt.v_free_count),
	    ptoa(cnt.v_free_count) / 1024);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
	vm_pager_bufferinit();

#ifdef SMP
	/*
	 * OK, enough kmem_alloc/malloc state should be up, lets get on with it!
	 */
	mp_start();			/* fire up the APs and APICs */
	mp_announce();
#endif  /* SMP */
}

int
register_netisr(num, handler)
	int num;
	netisr_t *handler;
{
	
	if (num < 0 || num >= (sizeof(netisrs)/sizeof(*netisrs)) ) {
		printf("register_netisr: bad isr number: %d\n", num);
		return (EINVAL);
	}
	netisrs[num] = handler;
	return (0);
}

void
netisr_sysinit(data)
	void *data;
{
	const struct netisrtab *nit;

	nit = (const struct netisrtab *)data;
	register_netisr(nit->nit_num, nit->nit_isr);
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * at top to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct trapframe *regs;
	register struct sigframe *fp;
	struct sigframe sf;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;

	regs = p->p_md.md_regs;
        oonstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;
	/*
	 * Allocate and validate space for the signal handler context.
	 */
        if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp +
		    psp->ps_sigstk.ss_size - sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else {
		fp = (struct sigframe *)regs->tf_esp - 1;
	}

	/*
	 * grow() will return FALSE if the fp will not fit inside the stack
	 *	and the stack can not be grown. useracc will return FALSE
	 *	if access is denied.
	 */
	if ((grow_stack (p, (int)fp) == FALSE) ||
	    (useracc((caddr_t)fp, sizeof(struct sigframe), B_WRITE) == FALSE)) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		printf("process %d has trashed its stack\n", p->p_pid);
#endif
		SIGACTION(p, SIGILL) = SIG_DFL;
		sig = sigmask(SIGILL);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, SIGILL);
		return;
	}

	/*
	 * Build the argument list for the signal handler.
	 */
	if (p->p_sysent->sv_sigtbl) {
		if (sig < p->p_sysent->sv_sigsize)
			sig = p->p_sysent->sv_sigtbl[sig];
		else
			sig = p->p_sysent->sv_sigsize + 1;
	}
	sf.sf_signum = sig;
	sf.sf_scp = (register_t)&fp->sf_siginfo.si_sc;
 	if (p->p_sigacts->ps_siginfo & sigmask(sig)) {
		/*
		 * Signal handler installed with SA_SIGINFO.
		 */
		sf.sf_arg2 = (register_t)&fp->sf_siginfo;
		sf.sf_siginfo.si_signo = sig;
		sf.sf_siginfo.si_code = code;
		sf.sf_ahu.sf_action = (__siginfohandler_t *)catcher;
	} else {
		/* 
		 * Old FreeBSD-style arguments.
		 */
		sf.sf_arg2 = code;
		sf.sf_ahu.sf_handler = catcher;
	}

	sf.sf_addr = (char *) regs->tf_err;

	/* save scratch registers */
	sf.sf_siginfo.si_sc.sc_eax = regs->tf_eax;
	sf.sf_siginfo.si_sc.sc_ebx = regs->tf_ebx;
	sf.sf_siginfo.si_sc.sc_ecx = regs->tf_ecx;
	sf.sf_siginfo.si_sc.sc_edx = regs->tf_edx;
	sf.sf_siginfo.si_sc.sc_esi = regs->tf_esi;
	sf.sf_siginfo.si_sc.sc_edi = regs->tf_edi;
	sf.sf_siginfo.si_sc.sc_cs = regs->tf_cs;
	sf.sf_siginfo.si_sc.sc_ds = regs->tf_ds;
	sf.sf_siginfo.si_sc.sc_ss = regs->tf_ss;
	sf.sf_siginfo.si_sc.sc_es = regs->tf_es;
	sf.sf_siginfo.si_sc.sc_fs = regs->tf_fs;
	sf.sf_siginfo.si_sc.sc_gs = rgs();
	sf.sf_siginfo.si_sc.sc_isp = regs->tf_isp;

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_siginfo.si_sc.sc_onstack = oonstack;
	sf.sf_siginfo.si_sc.sc_mask = mask;
	sf.sf_siginfo.si_sc.sc_sp = regs->tf_esp;
	sf.sf_siginfo.si_sc.sc_fp = regs->tf_ebp;
	sf.sf_siginfo.si_sc.sc_pc = regs->tf_eip;
	sf.sf_siginfo.si_sc.sc_ps = regs->tf_eflags;
	sf.sf_siginfo.si_sc.sc_trapno = regs->tf_trapno;
	sf.sf_siginfo.si_sc.sc_err = regs->tf_err;

	/*
	 * If we're a vm86 process, we want to save the segment registers.
	 * We also change eflags to be our emulated eflags, not the actual
	 * eflags.
	 */
	if (regs->tf_eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *)regs;
		struct vm86_kernel *vm86 = &p->p_addr->u_pcb.pcb_ext->ext_vm86;

		sf.sf_siginfo.si_sc.sc_gs = tf->tf_vm86_gs;
		sf.sf_siginfo.si_sc.sc_fs = tf->tf_vm86_fs;
		sf.sf_siginfo.si_sc.sc_es = tf->tf_vm86_es;
		sf.sf_siginfo.si_sc.sc_ds = tf->tf_vm86_ds;

		if (vm86->vm86_has_vme == 0)
			sf.sf_siginfo.si_sc.sc_ps = (tf->tf_eflags & ~(PSL_VIF | PSL_VIP))
			    | (vm86->vm86_eflags & (PSL_VIF | PSL_VIP));

		/*
		 * We should never have PSL_T set when returning from vm86
		 * mode.  It may be set here if we deliver a signal before
		 * getting to vm86 mode, so turn it off.
		 *
		 * Clear PSL_NT to inhibit T_TSSFLT faults on return from
		 * syscalls made by the signal handler.  This just avoids
		 * wasting time for our lazy fixup of such faults.  PSL_NT
		 * does nothing in vm86 mode, but vm86 programs can set it
		 * almost legitimately in probes for old cpu types.
		 */
		tf->tf_eflags &= ~(PSL_VM | PSL_NT | PSL_T | PSL_VIF | PSL_VIP);
	}

	/*
	 * Copy the sigframe out to the user's stack.
	 */
	if (copyout(&sf, fp, sizeof(struct sigframe)) != 0) {
		/*
		 * Something is wrong with the stack pointer.
		 * ...Kill the process.
		 */
		sigexit(p, SIGILL);
	}

	regs->tf_esp = (int)fp;
	regs->tf_eip = PS_STRINGS - *(p->p_sysent->sv_szsigcode);
	regs->tf_cs = _ucodesel;
	regs->tf_ds = _udatasel;
	regs->tf_es = _udatasel;
	regs->tf_fs = _udatasel;
	regs->tf_ss = _udatasel;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * state to gain improper privileges.
 */
int
sigreturn(p, uap)
	struct proc *p;
	struct sigreturn_args /* {
		struct sigcontext *sigcntxp;
	} */ *uap;
{
	register struct sigcontext *scp;
	register struct sigframe *fp;
	register struct trapframe *regs = p->p_md.md_regs;
	int eflags;

	/*
	 * (XXX old comment) regs->tf_esp points to the return address.
	 * The user scp pointer is above that.
	 * The return address is faked in the signal trampoline code
	 * for consistency.
	 */
	scp = uap->sigcntxp;
	fp = (struct sigframe *)
	     ((caddr_t)scp - offsetof(struct sigframe, sf_siginfo.si_sc));

	if (useracc((caddr_t)fp, sizeof (*fp), B_WRITE) == 0)
		return(EFAULT);

	eflags = scp->sc_ps;
	if (eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *)regs;
		struct vm86_kernel *vm86;

		/*
		 * if pcb_ext == 0 or vm86_inited == 0, the user hasn't
		 * set up the vm86 area, and we can't enter vm86 mode.
		 */
		if (p->p_addr->u_pcb.pcb_ext == 0)
			return (EINVAL);
		vm86 = &p->p_addr->u_pcb.pcb_ext->ext_vm86;
		if (vm86->vm86_inited == 0)
			return (EINVAL);

		/* go back to user mode if both flags are set */
		if ((eflags & PSL_VIP) && (eflags & PSL_VIF))
			trapsignal(p, SIGBUS, 0);

		if (vm86->vm86_has_vme) {
			eflags = (tf->tf_eflags & ~VME_USERCHANGE) |
			    (eflags & VME_USERCHANGE) | PSL_VM;
		} else {
			vm86->vm86_eflags = eflags;	/* save VIF, VIP */
			eflags = (tf->tf_eflags & ~VM_USERCHANGE) |					    (eflags & VM_USERCHANGE) | PSL_VM;
		}
		tf->tf_vm86_ds = scp->sc_ds;
		tf->tf_vm86_es = scp->sc_es;
		tf->tf_vm86_fs = scp->sc_fs;
		tf->tf_vm86_gs = scp->sc_gs;
		tf->tf_ds = _udatasel;
		tf->tf_es = _udatasel;
		tf->tf_fs = _udatasel;
	} else {
		/*
		 * Don't allow users to change privileged or reserved flags.
		 */
#define	EFLAGS_SECURE(ef, oef)	((((ef) ^ (oef)) & ~PSL_USERCHANGE) == 0)
		/*
		 * XXX do allow users to change the privileged flag PSL_RF.
		 * The cpu sets PSL_RF in tf_eflags for faults.  Debuggers
		 * should sometimes set it there too.  tf_eflags is kept in
		 * the signal context during signal handling and there is no
		 * other place to remember it, so the PSL_RF bit may be
		 * corrupted by the signal handler without us knowing.
		 * Corruption of the PSL_RF bit at worst causes one more or
		 * one less debugger trap, so allowing it is fairly harmless.
		 */
		if (!EFLAGS_SECURE(eflags & ~PSL_RF, regs->tf_eflags & ~PSL_RF)) {
#ifdef DEBUG
	    		printf("sigreturn: eflags = 0x%x\n", eflags);
#endif
	    		return(EINVAL);
		}

		/*
		 * Don't allow users to load a valid privileged %cs.  Let the
		 * hardware check for invalid selectors, excess privilege in
		 * other selectors, invalid %eip's and invalid %esp's.
		 */
#define	CS_SECURE(cs)	(ISPL(cs) == SEL_UPL)
		if (!CS_SECURE(scp->sc_cs)) {
#ifdef DEBUG
    			printf("sigreturn: cs = 0x%x\n", scp->sc_cs);
#endif
			trapsignal(p, SIGBUS, T_PROTFLT);
			return(EINVAL);
		}
		regs->tf_ds = scp->sc_ds;
		regs->tf_es = scp->sc_es;
		regs->tf_fs = scp->sc_fs;
	}

	/* restore scratch registers */
	regs->tf_eax = scp->sc_eax;
	regs->tf_ebx = scp->sc_ebx;
	regs->tf_ecx = scp->sc_ecx;
	regs->tf_edx = scp->sc_edx;
	regs->tf_esi = scp->sc_esi;
	regs->tf_edi = scp->sc_edi;
	regs->tf_cs = scp->sc_cs;
	regs->tf_ss = scp->sc_ss;
	regs->tf_isp = scp->sc_isp;

	if (useracc((caddr_t)scp, sizeof (*scp), B_WRITE) == 0)
		return(EINVAL);

	if (scp->sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = scp->sc_mask & ~sigcantmask;
	regs->tf_ebp = scp->sc_fp;
	regs->tf_esp = scp->sc_sp;
	regs->tf_eip = scp->sc_pc;
	regs->tf_eflags = eflags;
	return(EJUSTRETURN);
}

/*
 * Machine dependent boot() routine
 *
 * I haven't seen anything to put here yet
 * Possibly some stuff might be grafted back here from boot()
 */
void
cpu_boot(int howto)
{
}

/*
 * Shutdown the CPU as much as possible
 */
void
cpu_halt(void)
{
	for (;;)
		__asm__ ("hlt");
}

/*
 * Clear registers on exec
 */
void
setregs(p, entry, stack, ps_strings)
	struct proc *p;
	u_long entry;
	u_long stack;
	u_long ps_strings;
{
	struct trapframe *regs = p->p_md.md_regs;
	struct pcb *pcb = &p->p_addr->u_pcb;

#ifdef USER_LDT
	/* was i386_user_cleanup() in NetBSD */
	if (pcb->pcb_ldt) {
		if (pcb == curpcb) {
			lldt(_default_ldt);
			currentldt = _default_ldt;
		}
		kmem_free(kernel_map, (vm_offset_t)pcb->pcb_ldt,
			pcb->pcb_ldt_len * sizeof(union descriptor));
		pcb->pcb_ldt_len = (int)pcb->pcb_ldt = 0;
 	}
#endif
  
	bzero((char *)regs, sizeof(struct trapframe));
	regs->tf_eip = entry;
	regs->tf_esp = stack;
	regs->tf_eflags = PSL_USER | (regs->tf_eflags & PSL_T);
	regs->tf_ss = _udatasel;
	regs->tf_ds = _udatasel;
	regs->tf_es = _udatasel;
	regs->tf_fs = _udatasel;
	regs->tf_cs = _ucodesel;

	/* PS_STRINGS value for BSD/OS binaries.  It is 0 for non-BSD/OS. */
	regs->tf_ebx = ps_strings;

	/* reset %gs as well */
	pcb->pcb_gs = _udatasel;
	if (pcb == curpcb) {
		load_gs(_udatasel);
	}

	/*
	 * Initialize the math emulator (if any) for the current process.
	 * Actually, just clear the bit that says that the emulator has
	 * been initialized.  Initialization is delayed until the process
	 * traps to the emulator (if it is done at all) mainly because
	 * emulators don't provide an entry point for initialization.
	 */
	p->p_addr->u_pcb.pcb_flags &= ~FP_SOFTFP;

	/*
	 * Arrange to trap the next npx or `fwait' instruction (see npx.c
	 * for why fwait must be trapped at least if there is an npx or an
	 * emulator).  This is mainly to handle the case where npx0 is not
	 * configured, since the npx routines normally set up the trap
	 * otherwise.  It should be done only at boot time, but doing it
	 * here allows modifying `npx_exists' for testing the emulator on
	 * systems with an npx.
	 */
	load_cr0(rcr0() | CR0_MP | CR0_TS);

#if NNPX > 0
	/* Initialize the npx (if any) for the current process. */
	npxinit(__INITIAL_NPXCW__);
#endif

      /*
       * XXX - Linux emulator
       * Make sure sure edx is 0x0 on entry. Linux binaries depend
       * on it.
       */
      p->p_retval[1] = 0;
}

static int
sysctl_machdep_adjkerntz SYSCTL_HANDLER_ARGS
{
	int error;
	error = sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2,
		req);
	if (!error && req->newptr)
		resettodr();
	return (error);
}

SYSCTL_PROC(_machdep, CPU_ADJKERNTZ, adjkerntz, CTLTYPE_INT|CTLFLAG_RW,
	&adjkerntz, 0, sysctl_machdep_adjkerntz, "I", "");

SYSCTL_INT(_machdep, CPU_DISRTCSET, disable_rtc_set,
	CTLFLAG_RW, &disable_rtc_set, 0, "");

SYSCTL_STRUCT(_machdep, CPU_BOOTINFO, bootinfo, 
	CTLFLAG_RD, &bootinfo, bootinfo, "");

SYSCTL_INT(_machdep, CPU_WALLCLOCK, wall_cmos_clock,
	CTLFLAG_RW, &wall_cmos_clock, 0, "");

/*
 * Initialize 386 and configure to run kernel
 */

/*
 * Initialize segments & interrupt table
 */

int _default_ldt;
#ifdef SMP
union descriptor gdt[NGDT * NCPU];	/* global descriptor table */
#else
union descriptor gdt[NGDT];		/* global descriptor table */
#endif
static struct gate_descriptor idt0[NIDT];
struct gate_descriptor *idt = &idt0[0];	/* interrupt descriptor table */
union descriptor ldt[NLDT];		/* local descriptor table */
#ifdef SMP
/* table descriptors - used to load tables by microp */
struct region_descriptor r_gdt, r_idt;
#endif

#ifndef SMP
extern struct segment_descriptor common_tssd, *tss_gdt;
#endif
int private_tss;			/* flag indicating private tss */

#if defined(I586_CPU) && !defined(NO_F00F_HACK)
extern int has_f00f_bug;
#endif

static struct i386tss dblfault_tss;
static char dblfault_stack[PAGE_SIZE];

extern  struct user *proc0paddr;


/* software prototypes -- in more palatable form */
struct soft_segment_descriptor gdt_segs[] = {
/* GNULL_SEL	0 Null Descriptor */
{	0x0,			/* segment base address  */
	0x0,			/* length */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
/* GCODE_SEL	1 Code Descriptor for kernel */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMERA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GDATA_SEL	2 Data Descriptor for kernel */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GPRIV_SEL	3 SMP Per-Processor Private Data Descriptor */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GPROC0_SEL	4 Proc 0 Tss Descriptor */
{
	0x0,			/* segment base address */
	sizeof(struct i386tss)-1,/* length - all address space */
	SDT_SYS386TSS,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
/* GLDT_SEL	5 LDT Descriptor */
{	(int) ldt,		/* segment base address  */
	sizeof(ldt)-1,		/* length - all address space */
	SDT_SYSLDT,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
/* GUSERLDT_SEL	6 User LDT Descriptor per process */
{	(int) ldt,		/* segment base address  */
	(512 * sizeof(union descriptor)-1),		/* length */
	SDT_SYSLDT,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
/* GTGATE_SEL	7 Null Descriptor - Placeholder */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
/* GBIOSLOWMEM_SEL 8 BIOS access to realmode segment 0x40, must be #8 in GDT */
{	0x400,			/* segment base address */
	0xfffff,		/* length */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GPANIC_SEL	9 Panic Tss Descriptor */
{	(int) &dblfault_tss,	/* segment base address  */
	sizeof(struct i386tss)-1,/* length - all address space */
	SDT_SYS386TSS,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
/* GBIOSCODE32_SEL 10 BIOS 32-bit interface (32bit Code) */
{	0,			/* segment base address (overwritten)  */
	0xfffff,		/* length */
	SDT_MEMERA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GBIOSCODE16_SEL 11 BIOS 32-bit interface (16bit Code) */
{	0,			/* segment base address (overwritten)  */
	0xfffff,		/* length */
	SDT_MEMERA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GBIOSDATA_SEL 12 BIOS 32-bit interface (Data) */
{	0,			/* segment base address (overwritten) */
	0xfffff,		/* length */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GBIOSUTIL_SEL 13 BIOS 16-bit interface (Utility) */
{	0,			/* segment base address (overwritten) */
	0xfffff,		/* length */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
/* GBIOSARGS_SEL 14 BIOS 16-bit interface (Arguments) */
{	0,			/* segment base address (overwritten) */
	0xfffff,		/* length */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
};

static struct soft_segment_descriptor ldt_segs[] = {
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Code Descriptor for user */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMERA,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Data Descriptor for user */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMRWA,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
};

void
setidt(idx, func, typ, dpl, selec)
	int idx;
	inthand_t *func;
	int typ;
	int dpl;
	int selec;
{
	struct gate_descriptor *ip;

	ip = idt + idx;
	ip->gd_looffset = (int)func;
	ip->gd_selector = selec;
	ip->gd_stkcpy = 0;
	ip->gd_xx = 0;
	ip->gd_type = typ;
	ip->gd_dpl = dpl;
	ip->gd_p = 1;
	ip->gd_hioffset = ((int)func)>>16 ;
}

#define	IDTVEC(name)	__CONCAT(X,name)

extern inthand_t
	IDTVEC(div), IDTVEC(dbg), IDTVEC(nmi), IDTVEC(bpt), IDTVEC(ofl),
	IDTVEC(bnd), IDTVEC(ill), IDTVEC(dna), IDTVEC(fpusegm),
	IDTVEC(tss), IDTVEC(missing), IDTVEC(stk), IDTVEC(prot),
	IDTVEC(page), IDTVEC(mchk), IDTVEC(rsvd), IDTVEC(fpu), IDTVEC(align),
	IDTVEC(syscall), IDTVEC(int0x80_syscall);

void
sdtossd(sd, ssd)
	struct segment_descriptor *sd;
	struct soft_segment_descriptor *ssd;
{
	ssd->ssd_base  = (sd->sd_hibase << 24) | sd->sd_lobase;
	ssd->ssd_limit = (sd->sd_hilimit << 16) | sd->sd_lolimit;
	ssd->ssd_type  = sd->sd_type;
	ssd->ssd_dpl   = sd->sd_dpl;
	ssd->ssd_p     = sd->sd_p;
	ssd->ssd_def32 = sd->sd_def32;
	ssd->ssd_gran  = sd->sd_gran;
}

#define PHYSMAP_SIZE	(2 * 8)

/*
 * Populate the (physmap) array with base/bound pairs describing the
 * available physical memory in the system, then test this memory and
 * build the phys_avail array describing the actually-available memory.
 *
 * If we cannot accurately determine the physical memory map, then use
 * value from the 0xE801 call, and failing that, the RTC.
 *
 * Total memory size may be set by the kernel environment variable
 * hw.physmem or the compile-time define MAXMEM.
 */
static void
getmemsize(int first)
{
	int i, physmap_idx, pa_indx;
	u_int basemem, extmem;
	struct vm86frame vmf;
	struct vm86context vmc;
	vm_offset_t pa, physmap[PHYSMAP_SIZE];
	pt_entry_t pte;
	const char *cp;
	struct {
		u_int64_t base;
		u_int64_t length;
		u_int32_t type;
	} *smap;

	bzero(&vmf, sizeof(struct vm86frame));
	bzero(physmap, sizeof(physmap));

	/*
	 * Perform "base memory" related probes & setup
	 */
	vm86_intcall(0x12, &vmf);
	basemem = vmf.vmf_ax;
	if (basemem > 640) {
		printf("Preposterous BIOS basemem of %uK, truncating to 640K\n",
			basemem);
		basemem = 640;
	}

	/*
	 * XXX if biosbasemem is now < 640, there is a `hole'
	 * between the end of base memory and the start of
	 * ISA memory.  The hole may be empty or it may
	 * contain BIOS code or data.  Map it read/write so
	 * that the BIOS can write to it.  (Memory from 0 to
	 * the physical end of the kernel is mapped read-only
	 * to begin with and then parts of it are remapped.
	 * The parts that aren't remapped form holes that
	 * remain read-only and are unused by the kernel.
	 * The base memory area is below the physical end of
	 * the kernel and right now forms a read-only hole.
	 * The part of it from PAGE_SIZE to
	 * (trunc_page(biosbasemem * 1024) - 1) will be
	 * remapped and used by the kernel later.)
	 *
	 * This code is similar to the code used in
	 * pmap_mapdev, but since no memory needs to be
	 * allocated we simply change the mapping.
	 */
	for (pa = trunc_page(basemem * 1024);
	     pa < ISA_HOLE_START; pa += PAGE_SIZE) {
		pte = (pt_entry_t)vtopte(pa + KERNBASE);
		*pte = pa | PG_RW | PG_V;
	}

	/*
	 * if basemem != 640, map pages r/w into vm86 page table so 
	 * that the bios can scribble on it.
	 */
	pte = (pt_entry_t)vm86paddr;
	for (i = basemem / 4; i < 160; i++)
		pte[i] = (i << PAGE_SHIFT) | PG_V | PG_RW | PG_U;

	/*
	 * map page 1 R/W into the kernel page table so we can use it
	 * as a buffer.  The kernel will unmap this page later.
	 */
	pte = (pt_entry_t)vtopte(KERNBASE + (1 << PAGE_SHIFT));
	*pte = (1 << PAGE_SHIFT) | PG_RW | PG_V;

	/*
	 * get memory map with INT 15:E820
	 */
#define SMAPSIZ 	sizeof(*smap)
#define SMAP_SIG	0x534D4150			/* 'SMAP' */

	vmc.npages = 0;
	smap = (void *)vm86_addpage(&vmc, 1, KERNBASE + (1 << PAGE_SHIFT));
	vm86_getptr(&vmc, (vm_offset_t)smap, &vmf.vmf_es, &vmf.vmf_di);

	physmap_idx = 0;
	vmf.vmf_ebx = 0;
	do {
		vmf.vmf_eax = 0xE820;
		vmf.vmf_edx = SMAP_SIG;
		vmf.vmf_ecx = SMAPSIZ;
		i = vm86_datacall(0x15, &vmf, &vmc);
		if (i || vmf.vmf_eax != SMAP_SIG)
			break;
		if (boothowto & RB_VERBOSE)
			printf("SMAP type=%02x base=%08x %08x len=%08x %08x\n",
				smap->type,
				*(u_int32_t *)((char *)&smap->base + 4),
				(u_int32_t)smap->base,
				*(u_int32_t *)((char *)&smap->length + 4),
				(u_int32_t)smap->length);

		if (smap->type != 0x01)
			goto next_run;

		if (smap->length == 0)
			goto next_run;

		if (smap->base >= 0xffffffff) {
			printf("%uK of memory above 4GB ignored\n",
			    (u_int)(smap->length / 1024));
			goto next_run;
		}

		for (i = 0; i <= physmap_idx; i += 2) {
			if (smap->base < physmap[i + 1]) {
				if (boothowto & RB_VERBOSE)
					printf(
	"Overlapping or non-montonic memory region, ignoring second region\n");
				goto next_run;
			}
		}

		if (smap->base == physmap[physmap_idx + 1]) {
			physmap[physmap_idx + 1] += smap->length;
			goto next_run;
		}

		physmap_idx += 2;
		if (physmap_idx == PHYSMAP_SIZE) {
			printf(
		"Too many segments in the physical address map, giving up\n");
			break;
		}
		physmap[physmap_idx] = smap->base;
		physmap[physmap_idx + 1] = smap->base + smap->length;
next_run:
	} while (vmf.vmf_ebx != 0);

	if (physmap[1] != 0)
		goto physmap_done;

	/*
	 * If we failed above, try memory map with INT 15:E801
	 */
	vmf.vmf_ax = 0xE801;
	if (vm86_intcall(0x15, &vmf) == 0) {
		extmem = vmf.vmf_cx + vmf.vmf_dx * 64;
	} else {
#if 0
		vmf.vmf_ah = 0x88;
		vm86_intcall(0x15, &vmf);
		extmem = vmf.vmf_ax;
#else
		/*
		 * Prefer the RTC value for extended memory.
		 */
		extmem = rtcin(RTC_EXTLO) + (rtcin(RTC_EXTHI) << 8);
#endif
	}

	/*
	 * Special hack for chipsets that still remap the 384k hole when
	 * there's 16MB of memory - this really confuses people that
	 * are trying to use bus mastering ISA controllers with the
	 * "16MB limit"; they only have 16MB, but the remapping puts
	 * them beyond the limit.
	 *
	 * If extended memory is between 15-16MB (16-17MB phys address range),
	 *	chop it to 15MB.
	 */
	if ((extmem > 15 * 1024) && (extmem < 16 * 1024))
		extmem = 15 * 1024;

	physmap[0] = 0;
	physmap[1] = basemem * 1024;
	physmap_idx = 2;
	physmap[physmap_idx] = 0x100000;
	physmap[physmap_idx + 1] = physmap[physmap_idx] + extmem * 1024;

physmap_done:
	/*
	 * Now, physmap contains a map of physical memory.
	 */

#ifdef SMP
	/* make hole for AP bootstrap code */
	physmap[1] = mp_bootaddress(physmap[1] / 1024);

	/* look for the MP hardware - needed for apic addresses */
	mp_probe();
#endif

	/*
	 * Maxmem isn't the "maximum memory", it's one larger than the
	 * highest page of the physical address space.  It should be
	 * called something like "Maxphyspage".  We may adjust this 
	 * based on ``hw.physmem'' and the results of the memory test.
	 */
	Maxmem = atop(physmap[physmap_idx + 1]);

#ifdef MAXMEM
	Maxmem = MAXMEM / 4;
#endif

	/*
	 * hw.maxmem is a size in bytes; we also allow k, m, and g suffixes
	 * for the appropriate modifiers.  This overrides MAXMEM.
	 */
	if ((cp = getenv("hw.physmem")) != NULL) {
		u_int64_t AllowMem, sanity;
		const char *ep;

		sanity = AllowMem = strtouq(cp, &ep, 0);
		if ((ep != cp) && (*ep != 0)) {
			switch(*ep) {
			case 'g':
			case 'G':
				AllowMem <<= 10;
			case 'm':
			case 'M':
				AllowMem <<= 10;
			case 'k':
			case 'K':
				AllowMem <<= 10;
				break;
			default:
				AllowMem = sanity = 0;
			}
			if (AllowMem < sanity)
				AllowMem = 0;
		}
		if (AllowMem == 0)
			printf("Ignoring invalid memory size of '%s'\n", cp);
		else
			Maxmem = atop(AllowMem);
	}

	if (atop(physmap[physmap_idx + 1]) != Maxmem &&
	    (boothowto & RB_VERBOSE))
		printf("Physical memory use set to %uK\n", Maxmem * 4);

	/*
	 * If Maxmem has been increased beyond what the system has detected,
	 * extend the last memory segment to the new limit.
	 */ 
	if (atop(physmap[physmap_idx + 1]) < Maxmem)
		physmap[physmap_idx + 1] = ptoa(Maxmem);

	/* call pmap initialization to make new kernel address space */
	pmap_bootstrap(first, 0);

	/*
	 * Size up each available chunk of physical memory.
	 */
	physmap[0] = PAGE_SIZE;		/* mask off page 0 */
	pa_indx = 0;
	phys_avail[pa_indx++] = physmap[0];
	phys_avail[pa_indx] = physmap[0];
#if 0
	pte = (pt_entry_t)vtopte(KERNBASE);
#else
	pte = (pt_entry_t)CMAP1;
#endif

	/*
	 * physmap is in bytes, so when converting to page boundaries,
	 * round up the start address and round down the end address.
	 */
	for (i = 0; i <= physmap_idx; i += 2) {
		vm_offset_t end;

		end = ptoa(Maxmem);
		if (physmap[i + 1] < end)
			end = trunc_page(physmap[i + 1]);
		for (pa = round_page(physmap[i]); pa < end; pa += PAGE_SIZE) {
			int tmp, page_bad;
#if 0
			int *ptr = 0;
#else
			int *ptr = (int *)CADDR1;
#endif

			/*
			 * block out kernel memory as not available.
			 */
			if (pa >= 0x100000 && pa < first)
				continue;
	
			page_bad = FALSE;

			/*
			 * map page into kernel: valid, read/write,non-cacheable
			 */
			*pte = pa | PG_V | PG_RW | PG_N;
			invltlb();

			tmp = *(int *)ptr;
			/*
			 * Test for alternating 1's and 0's
			 */
			*(volatile int *)ptr = 0xaaaaaaaa;
			if (*(volatile int *)ptr != 0xaaaaaaaa) {
				page_bad = TRUE;
			}
			/*
			 * Test for alternating 0's and 1's
			 */
			*(volatile int *)ptr = 0x55555555;
			if (*(volatile int *)ptr != 0x55555555) {
			page_bad = TRUE;
			}
			/*
			 * Test for all 1's
			 */
			*(volatile int *)ptr = 0xffffffff;
			if (*(volatile int *)ptr != 0xffffffff) {
				page_bad = TRUE;
			}
			/*
			 * Test for all 0's
			 */
			*(volatile int *)ptr = 0x0;
			if (*(volatile int *)ptr != 0x0) {
				page_bad = TRUE;
			}
			/*
			 * Restore original value.
			 */
			*(int *)ptr = tmp;

			/*
			 * Adjust array of valid/good pages.
			 */
			if (page_bad == TRUE) {
				continue;
			}
			/*
			 * If this good page is a continuation of the
			 * previous set of good pages, then just increase
			 * the end pointer. Otherwise start a new chunk.
			 * Note that "end" points one higher than end,
			 * making the range >= start and < end.
			 * If we're also doing a speculative memory
			 * test and we at or past the end, bump up Maxmem
			 * so that we keep going. The first bad page
			 * will terminate the loop.
			 */
			if (phys_avail[pa_indx] == pa) {
				phys_avail[pa_indx] += PAGE_SIZE;
			} else {
				pa_indx++;
				if (pa_indx == PHYS_AVAIL_ARRAY_END) {
					printf("Too many holes in the physical address space, giving up\n");
					pa_indx--;
					break;
				}
				phys_avail[pa_indx++] = pa;	/* start */
				phys_avail[pa_indx] = pa + PAGE_SIZE;	/* end */
			}
			physmem++;
		}
	}
	*pte = 0;
	invltlb();

	/*
	 * XXX
	 * The last chunk must contain at least one page plus the message
	 * buffer to avoid complicating other code (message buffer address
	 * calculation, etc.).
	 */
	while (phys_avail[pa_indx - 1] + PAGE_SIZE +
	    round_page(MSGBUF_SIZE) >= phys_avail[pa_indx]) {
		physmem -= atop(phys_avail[pa_indx] - phys_avail[pa_indx - 1]);
		phys_avail[pa_indx--] = 0;
		phys_avail[pa_indx--] = 0;
	}

	Maxmem = atop(phys_avail[pa_indx]);

	/* Trim off space for the message buffer. */
	phys_avail[pa_indx] -= round_page(MSGBUF_SIZE);

	avail_end = phys_avail[pa_indx];
}

void
init386(first)
	int first;
{
	int x;
	struct gate_descriptor *gdp;
	int gsel_tss;
#ifndef SMP
	/* table descriptors - used to load tables by microp */
	struct region_descriptor r_gdt, r_idt;
#endif
	int off;

	/*
	 * Prevent lowering of the ipl if we call tsleep() early.
	 */
	safepri = cpl;

	proc0.p_addr = proc0paddr;

	atdevbase = ISA_HOLE_START + KERNBASE;

	if (bootinfo.bi_modulep) {
		preload_metadata = (caddr_t)bootinfo.bi_modulep + KERNBASE;
		preload_bootstrap_relocate(KERNBASE);
	}
	if (bootinfo.bi_envp)
		kern_envp = (caddr_t)bootinfo.bi_envp + KERNBASE;

	/*
	 * make gdt memory segments, the code segment goes up to end of the
	 * page with etext in it, the data segment goes to the end of
	 * the address space
	 */
	/*
	 * XXX text protection is temporarily (?) disabled.  The limit was
	 * i386_btop(round_page(etext)) - 1.
	 */
	gdt_segs[GCODE_SEL].ssd_limit = i386_btop(0) - 1;
	gdt_segs[GDATA_SEL].ssd_limit = i386_btop(0) - 1;
#ifdef SMP
	gdt_segs[GPRIV_SEL].ssd_limit =
		i386_btop(sizeof(struct privatespace)) - 1;
	gdt_segs[GPRIV_SEL].ssd_base = (int) &SMP_prvspace[0];
	gdt_segs[GPROC0_SEL].ssd_base =
		(int) &SMP_prvspace[0].globaldata.gd_common_tss;
	SMP_prvspace[0].globaldata.gd_prvspace = &SMP_prvspace[0];
#else
	gdt_segs[GPRIV_SEL].ssd_limit = i386_btop(0) - 1;
	gdt_segs[GPROC0_SEL].ssd_base = (int) &common_tss;
#endif

	for (x = 0; x < NGDT; x++) {
#ifdef BDE_DEBUGGER
		/* avoid overwriting db entries with APM ones */
		if (x >= GAPMCODE32_SEL && x <= GAPMDATA_SEL)
			continue;
#endif
		ssdtosd(&gdt_segs[x], &gdt[x].sd);
	}

	r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	r_gdt.rd_base =  (int) gdt;
	lgdt(&r_gdt);

	/* make ldt memory segments */
	/*
	 * The data segment limit must not cover the user area because we
	 * don't want the user area to be writable in copyout() etc. (page
	 * level protection is lost in kernel mode on 386's).  Also, we
	 * don't want the user area to be writable directly (page level
	 * protection of the user area is not available on 486's with
	 * CR0_WP set, because there is no user-read/kernel-write mode).
	 *
	 * XXX - VM_MAXUSER_ADDRESS is an end address, not a max.  And it
	 * should be spelled ...MAX_USER...
	 */
#define VM_END_USER_RW_ADDRESS	VM_MAXUSER_ADDRESS
	/*
	 * The code segment limit has to cover the user area until we move
	 * the signal trampoline out of the user area.  This is safe because
	 * the code segment cannot be written to directly.
	 */
#define VM_END_USER_R_ADDRESS	(VM_END_USER_RW_ADDRESS + UPAGES * PAGE_SIZE)
	ldt_segs[LUCODE_SEL].ssd_limit = i386_btop(VM_END_USER_R_ADDRESS) - 1;
	ldt_segs[LUDATA_SEL].ssd_limit = i386_btop(VM_END_USER_RW_ADDRESS) - 1;
	for (x = 0; x < sizeof ldt_segs / sizeof ldt_segs[0]; x++)
		ssdtosd(&ldt_segs[x], &ldt[x].sd);

	_default_ldt = GSEL(GLDT_SEL, SEL_KPL);
	lldt(_default_ldt);
#ifdef USER_LDT
	currentldt = _default_ldt;
#endif

	/* exceptions */
	for (x = 0; x < NIDT; x++)
		setidt(x, &IDTVEC(rsvd), SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(0, &IDTVEC(div),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(1, &IDTVEC(dbg),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(2, &IDTVEC(nmi),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
 	setidt(3, &IDTVEC(bpt),  SDT_SYS386TGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(4, &IDTVEC(ofl),  SDT_SYS386TGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(5, &IDTVEC(bnd),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(6, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(7, &IDTVEC(dna),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(8, 0,  SDT_SYSTASKGT, SEL_KPL, GSEL(GPANIC_SEL, SEL_KPL));
	setidt(9, &IDTVEC(fpusegm),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(10, &IDTVEC(tss),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(11, &IDTVEC(missing),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(12, &IDTVEC(stk),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(13, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(14, &IDTVEC(page),  SDT_SYS386IGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(15, &IDTVEC(rsvd),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(16, &IDTVEC(fpu),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(17, &IDTVEC(align), SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(18, &IDTVEC(mchk),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
 	setidt(0x80, &IDTVEC(int0x80_syscall),
			SDT_SYS386TGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));

	r_idt.rd_limit = sizeof(idt0) - 1;
	r_idt.rd_base = (int) idt;
	lidt(&r_idt);

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#include	"isa.h"
#if	NISA >0
	isa_defaultirq();
#endif
	rand_initialize();

#ifdef DDB
	kdb_init();
	if (boothowto & RB_KDB)
		Debugger("Boot flags requested debugger");
#endif

	finishidentcpu();	/* Final stage of CPU initialization */
	setidt(6, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(13, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	initializecpu();	/* Initialize CPU registers */

	/* make an initial tss so cpu can get interrupt stack on syscall! */
	common_tss.tss_esp0 = (int) proc0.p_addr + UPAGES*PAGE_SIZE - 16;
	common_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL) ;
	gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	private_tss = 0;
	tss_gdt = &gdt[GPROC0_SEL].sd;
	common_tssd = *tss_gdt;
	common_tss.tss_ioopt = (sizeof common_tss) << 16;
	ltr(gsel_tss);

	dblfault_tss.tss_esp = dblfault_tss.tss_esp0 = dblfault_tss.tss_esp1 =
	    dblfault_tss.tss_esp2 = (int) &dblfault_stack[sizeof(dblfault_stack)];
	dblfault_tss.tss_ss = dblfault_tss.tss_ss0 = dblfault_tss.tss_ss1 =
	    dblfault_tss.tss_ss2 = GSEL(GDATA_SEL, SEL_KPL);
	dblfault_tss.tss_cr3 = (int)IdlePTD;
	dblfault_tss.tss_eip = (int) dblfault_handler;
	dblfault_tss.tss_eflags = PSL_KERNEL;
	dblfault_tss.tss_ds = dblfault_tss.tss_es =
	    dblfault_tss.tss_gs = GSEL(GDATA_SEL, SEL_KPL);
	dblfault_tss.tss_fs = GSEL(GPRIV_SEL, SEL_KPL);
	dblfault_tss.tss_cs = GSEL(GCODE_SEL, SEL_KPL);
	dblfault_tss.tss_ldt = GSEL(GLDT_SEL, SEL_KPL);

	vm86_initialize();
	getmemsize(first);

	/* now running on new page tables, configured,and u/iom is accessible */

	/* Map the message buffer. */
	for (off = 0; off < round_page(MSGBUF_SIZE); off += PAGE_SIZE)
		pmap_kenter((vm_offset_t)msgbufp + off, avail_end + off);

	msgbufinit(msgbufp, MSGBUF_SIZE);

	/* make a call gate to reenter kernel with */
	gdp = &ldt[LSYS5CALLS_SEL].gd;

	x = (int) &IDTVEC(syscall);
	gdp->gd_looffset = x++;
	gdp->gd_selector = GSEL(GCODE_SEL,SEL_KPL);
	gdp->gd_stkcpy = 1;
	gdp->gd_type = SDT_SYS386CGT;
	gdp->gd_dpl = SEL_UPL;
	gdp->gd_p = 1;
	gdp->gd_hioffset = ((int) &IDTVEC(syscall)) >>16;

	/* XXX does this work? */
	ldt[LBSDICALLS_SEL] = ldt[LSYS5CALLS_SEL];
	ldt[LSOL26CALLS_SEL] = ldt[LSYS5CALLS_SEL];

	/* transfer to user mode */

	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc 0's pcb */
	proc0.p_addr->u_pcb.pcb_flags = 0;
	proc0.p_addr->u_pcb.pcb_cr3 = (int)IdlePTD;
#ifdef SMP
	proc0.p_addr->u_pcb.pcb_mpnest = 1;
#endif
	proc0.p_addr->u_pcb.pcb_ext = 0;
}

#if defined(I586_CPU) && !defined(NO_F00F_HACK)
static void f00f_hack(void *unused);
SYSINIT(f00f_hack, SI_SUB_INTRINSIC, SI_ORDER_FIRST, f00f_hack, NULL);

static void
f00f_hack(void *unused) {
	struct gate_descriptor *new_idt;
#ifndef SMP
	struct region_descriptor r_idt;
#endif
	vm_offset_t tmp;

	if (!has_f00f_bug)
		return;

	printf("Intel Pentium detected, installing workaround for F00F bug\n");

	r_idt.rd_limit = sizeof(idt0) - 1;

	tmp = kmem_alloc(kernel_map, PAGE_SIZE * 2);
	if (tmp == 0)
		panic("kmem_alloc returned 0");
	if (((unsigned int)tmp & (PAGE_SIZE-1)) != 0)
		panic("kmem_alloc returned non-page-aligned memory");
	/* Put the first seven entries in the lower page */
	new_idt = (struct gate_descriptor*)(tmp + PAGE_SIZE - (7*8));
	bcopy(idt, new_idt, sizeof(idt0));
	r_idt.rd_base = (int)new_idt;
	lidt(&r_idt);
	idt = new_idt;
	if (vm_map_protect(kernel_map, tmp, tmp + PAGE_SIZE,
			   VM_PROT_READ, FALSE) != KERN_SUCCESS)
		panic("vm_map_protect failed");
	return;
}
#endif /* defined(I586_CPU) && !NO_F00F_HACK */

int
ptrace_set_pc(p, addr)
	struct proc *p;
	unsigned long addr;
{
	p->p_md.md_regs->tf_eip = addr;
	return (0);
}

int
ptrace_single_step(p)
	struct proc *p;
{
	p->p_md.md_regs->tf_eflags |= PSL_T;
	return (0);
}

int ptrace_read_u_check(p, addr, len)
	struct proc *p;
	vm_offset_t addr;
	size_t len;
{
	vm_offset_t gap;

	if ((vm_offset_t) (addr + len) < addr)
		return EPERM;
	if ((vm_offset_t) (addr + len) <= sizeof(struct user))
		return 0;

	gap = (char *) p->p_md.md_regs - (char *) p->p_addr;
	
	if ((vm_offset_t) addr < gap)
		return EPERM;
	if ((vm_offset_t) (addr + len) <= 
	    (vm_offset_t) (gap + sizeof(struct trapframe)))
		return 0;
	return EPERM;
}

int ptrace_write_u(p, off, data)
	struct proc *p;
	vm_offset_t off;
	long data;
{
	struct trapframe frame_copy;
	vm_offset_t min;
	struct trapframe *tp;

	/*
	 * Privileged kernel state is scattered all over the user area.
	 * Only allow write access to parts of regs and to fpregs.
	 */
	min = (char *)p->p_md.md_regs - (char *)p->p_addr;
	if (off >= min && off <= min + sizeof(struct trapframe) - sizeof(int)) {
		tp = p->p_md.md_regs;
		frame_copy = *tp;
		*(int *)((char *)&frame_copy + (off - min)) = data;
		if (!EFLAGS_SECURE(frame_copy.tf_eflags, tp->tf_eflags) ||
		    !CS_SECURE(frame_copy.tf_cs))
			return (EINVAL);
		*(int*)((char *)p->p_addr + off) = data;
		return (0);
	}
	min = offsetof(struct user, u_pcb) + offsetof(struct pcb, pcb_savefpu);
	if (off >= min && off <= min + sizeof(struct save87) - sizeof(int)) {
		*(int*)((char *)p->p_addr + off) = data;
		return (0);
	}
	return (EFAULT);
}

int
fill_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct pcb *pcb;
	struct trapframe *tp;

	tp = p->p_md.md_regs;
	regs->r_fs = tp->tf_fs;
	regs->r_es = tp->tf_es;
	regs->r_ds = tp->tf_ds;
	regs->r_edi = tp->tf_edi;
	regs->r_esi = tp->tf_esi;
	regs->r_ebp = tp->tf_ebp;
	regs->r_ebx = tp->tf_ebx;
	regs->r_edx = tp->tf_edx;
	regs->r_ecx = tp->tf_ecx;
	regs->r_eax = tp->tf_eax;
	regs->r_eip = tp->tf_eip;
	regs->r_cs = tp->tf_cs;
	regs->r_eflags = tp->tf_eflags;
	regs->r_esp = tp->tf_esp;
	regs->r_ss = tp->tf_ss;
	pcb = &p->p_addr->u_pcb;
	regs->r_gs = pcb->pcb_gs;
	return (0);
}

int
set_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct pcb *pcb;
	struct trapframe *tp;

	tp = p->p_md.md_regs;
	if (!EFLAGS_SECURE(regs->r_eflags, tp->tf_eflags) ||
	    !CS_SECURE(regs->r_cs))
		return (EINVAL);
	tp->tf_fs = regs->r_fs;
	tp->tf_es = regs->r_es;
	tp->tf_ds = regs->r_ds;
	tp->tf_edi = regs->r_edi;
	tp->tf_esi = regs->r_esi;
	tp->tf_ebp = regs->r_ebp;
	tp->tf_ebx = regs->r_ebx;
	tp->tf_edx = regs->r_edx;
	tp->tf_ecx = regs->r_ecx;
	tp->tf_eax = regs->r_eax;
	tp->tf_eip = regs->r_eip;
	tp->tf_cs = regs->r_cs;
	tp->tf_eflags = regs->r_eflags;
	tp->tf_esp = regs->r_esp;
	tp->tf_ss = regs->r_ss;
	pcb = &p->p_addr->u_pcb;
	pcb->pcb_gs = regs->r_gs;
	return (0);
}

int
fill_fpregs(p, fpregs)
	struct proc *p;
	struct fpreg *fpregs;
{
	bcopy(&p->p_addr->u_pcb.pcb_savefpu, fpregs, sizeof *fpregs);
	return (0);
}

int
set_fpregs(p, fpregs)
	struct proc *p;
	struct fpreg *fpregs;
{
	bcopy(fpregs, &p->p_addr->u_pcb.pcb_savefpu, sizeof *fpregs);
	return (0);
}

int
fill_dbregs(p, dbregs)
	struct proc *p;
	struct dbreg *dbregs;
{
	struct pcb *pcb;

	pcb = &p->p_addr->u_pcb;
	dbregs->dr0 = pcb->pcb_dr0;
	dbregs->dr1 = pcb->pcb_dr1;
	dbregs->dr2 = pcb->pcb_dr2;
	dbregs->dr3 = pcb->pcb_dr3;
	dbregs->dr4 = 0;
	dbregs->dr5 = 0;
	dbregs->dr6 = pcb->pcb_dr6;
	dbregs->dr7 = pcb->pcb_dr7;
	return (0);
}

int
set_dbregs(p, dbregs)
	struct proc *p;
	struct dbreg *dbregs;
{
	struct pcb *pcb;

	pcb = &p->p_addr->u_pcb;

	/*
	 * Don't let a process set a breakpoint that is not within the
	 * process's address space.  If a process could do this, it
	 * could halt the system by setting a breakpoint in the kernel
	 * (if ddb was enabled).  Thus, we need to check to make sure
	 * that no breakpoints are being enabled for addresses outside
	 * process's address space, unless, perhaps, we were called by
	 * uid 0.
	 *
	 * XXX - what about when the watched area of the user's
	 * address space is written into from within the kernel
	 * ... wouldn't that still cause a breakpoint to be generated
	 * from within kernel mode?
	 */

	if (p->p_cred->pc_ucred->cr_uid != 0) {
		if (dbregs->dr7 & 0x3) {
			/* dr0 is enabled */
			if (dbregs->dr0 >= VM_MAXUSER_ADDRESS)
				return (EINVAL);
		}

		if (dbregs->dr7 & (0x3<<2)) {
			/* dr1 is enabled */
			if (dbregs->dr1 >= VM_MAXUSER_ADDRESS)
				return (EINVAL);
		}

		if (dbregs->dr7 & (0x3<<4)) {
			/* dr2 is enabled */
			if (dbregs->dr2 >= VM_MAXUSER_ADDRESS)
       				return (EINVAL);
		}

		if (dbregs->dr7 & (0x3<<6)) {
			/* dr3 is enabled */
			if (dbregs->dr3 >= VM_MAXUSER_ADDRESS)
				return (EINVAL);
		}
	}

	pcb->pcb_dr0 = dbregs->dr0;
	pcb->pcb_dr1 = dbregs->dr1;
	pcb->pcb_dr2 = dbregs->dr2;
	pcb->pcb_dr3 = dbregs->dr3;
	pcb->pcb_dr6 = dbregs->dr6;
	pcb->pcb_dr7 = dbregs->dr7;

	pcb->pcb_flags |= PCB_DBREGS;

	return (0);
}

#ifndef DDB
void
Debugger(const char *msg)
{
	printf("Debugger(\"%s\") called.\n", msg);
}
#endif /* no DDB */

#include <sys/disklabel.h>

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(struct buf *bp, struct disklabel *lp, int wlabel)
{
        struct partition *p = lp->d_partitions + dkpart(bp->b_dev);
        int labelsect = lp->d_partitions[0].p_offset;
        int maxsz = p->p_size,
                sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;

        /* overwriting disk label ? */
        /* XXX should also protect bootstrap in first 8K */
        if (bp->b_blkno + p->p_offset <= LABELSECTOR + labelsect &&
#if LABELSECTOR != 0
            bp->b_blkno + p->p_offset + sz > LABELSECTOR + labelsect &&
#endif
            (bp->b_flags & B_READ) == 0 && wlabel == 0) {
                bp->b_error = EROFS;
                goto bad;
        }

#if     defined(DOSBBSECTOR) && defined(notyet)
        /* overwriting master boot record? */
        if (bp->b_blkno + p->p_offset <= DOSBBSECTOR &&
            (bp->b_flags & B_READ) == 0 && wlabel == 0) {
                bp->b_error = EROFS;
                goto bad;
        }
#endif

        /* beyond partition? */
        if (bp->b_blkno < 0 || bp->b_blkno + sz > maxsz) {
                /* if exactly at end of disk, return an EOF */
                if (bp->b_blkno == maxsz) {
                        bp->b_resid = bp->b_bcount;
                        return(0);
                }
                /* or truncate if part of it fits */
                sz = maxsz - bp->b_blkno;
                if (sz <= 0) {
                        bp->b_error = EINVAL;
                        goto bad;
                }
                bp->b_bcount = sz << DEV_BSHIFT;
        }

        bp->b_pblkno = bp->b_blkno + p->p_offset;
        return(1);

bad:
        bp->b_flags |= B_ERROR;
        return(-1);
}

#ifdef DDB

/*
 * Provide inb() and outb() as functions.  They are normally only
 * available as macros calling inlined functions, thus cannot be
 * called inside DDB.
 *
 * The actual code is stolen from <machine/cpufunc.h>, and de-inlined.
 */

#undef inb
#undef outb

/* silence compiler warnings */
u_char inb(u_int);
void outb(u_int, u_char);

u_char
inb(u_int port)
{
	u_char	data;
	/*
	 * We use %%dx and not %1 here because i/o is done at %dx and not at
	 * %edx, while gcc generates inferior code (movw instead of movl)
	 * if we tell it to load (u_short) port.
	 */
	__asm __volatile("inb %%dx,%0" : "=a" (data) : "d" (port));
	return (data);
}

void
outb(u_int port, u_char data)
{
	u_char	al;
	/*
	 * Use an unnecessary assignment to help gcc's register allocator.
	 * This make a large difference for gcc-1.40 and a tiny difference
	 * for gcc-2.6.0.  For gcc-1.40, al had to be ``asm("ax")'' for
	 * best results.  gcc-2.6.0 can't handle this.
	 */
	al = data;
	__asm __volatile("outb %0,%%dx" : : "a" (al), "d" (port));
}

#endif /* DDB */
