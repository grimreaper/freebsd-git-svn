/* From: $NetBSD: pmap.c,v 1.148 2004/04/03 04:35:48 bsh Exp $ */
/*-
 * Copyright 2004 Olivier Houchard.
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2002-2003 Wasabi Systems, Inc.
 * Copyright (c) 2001 Richard Earnshaw
 * Copyright (c) 2001-2002 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *      This product includes software developed by Mark Brinicombe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * RiscBSD kernel project
 *
 * pmap.c
 *
 * Machine dependant vm stuff
 *
 * Created      : 20/09/94
 */

/*
 * Special compilation symbols
 * PMAP_DEBUG           - Build in pmap_debug_level code
 */
/* Include header files */

#include "opt_vm.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/vmmeter.h>
#include <sys/mman.h>
#include <sys/smp.h>
#include <sys/sched.h>

#include <vm/vm.h>
#include <vm/uma.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_extern.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <machine/md_var.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/pcb.h>

#ifdef PMAP_DEBUG
#define PDEBUG(_lev_,_stat_) \
        if (pmap_debug_level >= (_lev_)) \
                ((_stat_))
#define dprintf printf

int pmap_debug_level = 0;
#define PMAP_INLINE 
#else   /* PMAP_DEBUG */
#define PDEBUG(_lev_,_stat_) /* Nothing */
#define dprintf(x, arg...)
#define PMAP_INLINE __inline
#endif  /* PMAP_DEBUG */

extern struct pv_addr systempage;
/*
 * Internal function prototypes
 */
static void pmap_free_pv_entry (pv_entry_t);
static pv_entry_t pmap_get_pv_entry(void);

static void		pmap_enter_locked(pmap_t, vm_offset_t, vm_page_t,
    vm_prot_t, boolean_t, int);
static void		pmap_vac_me_harder(struct vm_page *, pmap_t,
    vm_offset_t);
static void		pmap_vac_me_kpmap(struct vm_page *, pmap_t, 
    vm_offset_t);
static void		pmap_vac_me_user(struct vm_page *, pmap_t, vm_offset_t);
static void		pmap_alloc_l1(pmap_t);
static void		pmap_free_l1(pmap_t);
static void		pmap_use_l1(pmap_t);

static int		pmap_clearbit(struct vm_page *, u_int);

static struct l2_bucket *pmap_get_l2_bucket(pmap_t, vm_offset_t);
static struct l2_bucket *pmap_alloc_l2_bucket(pmap_t, vm_offset_t);
static void		pmap_free_l2_bucket(pmap_t, struct l2_bucket *, u_int);
static vm_offset_t	kernel_pt_lookup(vm_paddr_t);

static MALLOC_DEFINE(M_VMPMAP, "pmap", "PMAP L1");

vm_offset_t virtual_avail;	/* VA of first avail page (after kernel bss) */
vm_offset_t virtual_end;	/* VA of last avail page (end of kernel AS) */
vm_offset_t pmap_curmaxkvaddr;
vm_paddr_t kernel_l1pa;

extern void *end;
vm_offset_t kernel_vm_end = 0;

struct pmap kernel_pmap_store;
pmap_t kernel_pmap;

static pt_entry_t *csrc_pte, *cdst_pte;
static vm_offset_t csrcp, cdstp;
static struct mtx cmtx;

static void		pmap_init_l1(struct l1_ttable *, pd_entry_t *);
/*
 * These routines are called when the CPU type is identified to set up
 * the PTE prototypes, cache modes, etc.
 *
 * The variables are always here, just in case LKMs need to reference
 * them (though, they shouldn't).
 */

pt_entry_t	pte_l1_s_cache_mode;
pt_entry_t	pte_l1_s_cache_mode_pt;
pt_entry_t	pte_l1_s_cache_mask;

pt_entry_t	pte_l2_l_cache_mode;
pt_entry_t	pte_l2_l_cache_mode_pt;
pt_entry_t	pte_l2_l_cache_mask;

pt_entry_t	pte_l2_s_cache_mode;
pt_entry_t	pte_l2_s_cache_mode_pt;
pt_entry_t	pte_l2_s_cache_mask;

pt_entry_t	pte_l2_s_prot_u;
pt_entry_t	pte_l2_s_prot_w;
pt_entry_t	pte_l2_s_prot_mask;

pt_entry_t	pte_l1_s_proto;
pt_entry_t	pte_l1_c_proto;
pt_entry_t	pte_l2_s_proto;

void		(*pmap_copy_page_func)(vm_paddr_t, vm_paddr_t);
void		(*pmap_zero_page_func)(vm_paddr_t, int, int);
/*
 * Which pmap is currently 'live' in the cache
 *
 * XXXSCW: Fix for SMP ...
 */
union pmap_cache_state *pmap_cache_state;

struct msgbuf *msgbufp = 0;

extern void bcopy_page(vm_offset_t, vm_offset_t);
extern void bzero_page(vm_offset_t);

extern vm_offset_t alloc_firstaddr;

char *_tmppt;

/*
 * Metadata for L1 translation tables.
 */
struct l1_ttable {
	/* Entry on the L1 Table list */
	SLIST_ENTRY(l1_ttable) l1_link;

	/* Entry on the L1 Least Recently Used list */
	TAILQ_ENTRY(l1_ttable) l1_lru;

	/* Track how many domains are allocated from this L1 */
	volatile u_int l1_domain_use_count;

	/*
	 * A free-list of domain numbers for this L1.
	 * We avoid using ffs() and a bitmap to track domains since ffs()
	 * is slow on ARM.
	 */
	u_int8_t l1_domain_first;
	u_int8_t l1_domain_free[PMAP_DOMAINS];

	/* Physical address of this L1 page table */
	vm_paddr_t l1_physaddr;

	/* KVA of this L1 page table */
	pd_entry_t *l1_kva;
};

/*
 * Convert a virtual address into its L1 table index. That is, the
 * index used to locate the L2 descriptor table pointer in an L1 table.
 * This is basically used to index l1->l1_kva[].
 *
 * Each L2 descriptor table represents 1MB of VA space.
 */
#define	L1_IDX(va)		(((vm_offset_t)(va)) >> L1_S_SHIFT)

/*
 * L1 Page Tables are tracked using a Least Recently Used list.
 *  - New L1s are allocated from the HEAD.
 *  - Freed L1s are added to the TAIl.
 *  - Recently accessed L1s (where an 'access' is some change to one of
 *    the userland pmaps which owns this L1) are moved to the TAIL.
 */
static TAILQ_HEAD(, l1_ttable) l1_lru_list;
/*
 * A list of all L1 tables
 */
static SLIST_HEAD(, l1_ttable) l1_list;
static struct mtx l1_lru_lock;

/*
 * The l2_dtable tracks L2_BUCKET_SIZE worth of L1 slots.
 *
 * This is normally 16MB worth L2 page descriptors for any given pmap.
 * Reference counts are maintained for L2 descriptors so they can be
 * freed when empty.
 */
struct l2_dtable {
	/* The number of L2 page descriptors allocated to this l2_dtable */
	u_int l2_occupancy;

	/* List of L2 page descriptors */
	struct l2_bucket {
		pt_entry_t *l2b_kva;	/* KVA of L2 Descriptor Table */
		vm_paddr_t l2b_phys;	/* Physical address of same */
		u_short l2b_l1idx;	/* This L2 table's L1 index */
		u_short l2b_occupancy;	/* How many active descriptors */
	} l2_bucket[L2_BUCKET_SIZE];
};

/* pmap_kenter_internal flags */
#define KENTER_CACHE	0x1
#define KENTER_USER	0x2

/*
 * Given an L1 table index, calculate the corresponding l2_dtable index
 * and bucket index within the l2_dtable.
 */
#define	L2_IDX(l1idx)		(((l1idx) >> L2_BUCKET_LOG2) & \
				 (L2_SIZE - 1))
#define	L2_BUCKET(l1idx)	((l1idx) & (L2_BUCKET_SIZE - 1))

/*
 * Given a virtual address, this macro returns the
 * virtual address required to drop into the next L2 bucket.
 */
#define	L2_NEXT_BUCKET(va)	(((va) & L1_S_FRAME) + L1_S_SIZE)

/*
 * L2 allocation.
 */
#define	pmap_alloc_l2_dtable()		\
		(void*)uma_zalloc(l2table_zone, M_NOWAIT|M_USE_RESERVE)
#define	pmap_free_l2_dtable(l2)		\
		uma_zfree(l2table_zone, l2)

/*
 * We try to map the page tables write-through, if possible.  However, not
 * all CPUs have a write-through cache mode, so on those we have to sync
 * the cache when we frob page tables.
 *
 * We try to evaluate this at compile time, if possible.  However, it's
 * not always possible to do that, hence this run-time var.
 */
int	pmap_needs_pte_sync;

/*
 * Macro to determine if a mapping might be resident in the
 * instruction cache and/or TLB
 */
#define	PV_BEEN_EXECD(f)  (((f) & (PVF_REF | PVF_EXEC)) == (PVF_REF | PVF_EXEC))

/*
 * Macro to determine if a mapping might be resident in the
 * data cache and/or TLB
 */
#define	PV_BEEN_REFD(f)   (((f) & PVF_REF) != 0)

#ifndef PMAP_SHPGPERPROC
#define PMAP_SHPGPERPROC 200
#endif

#define pmap_is_current(pm)	((pm) == pmap_kernel() || \
            curproc->p_vmspace->vm_map.pmap == (pm))
static uma_zone_t pvzone;
uma_zone_t l2zone;
static uma_zone_t l2table_zone;
static vm_offset_t pmap_kernel_l2dtable_kva;
static vm_offset_t pmap_kernel_l2ptp_kva;
static vm_paddr_t pmap_kernel_l2ptp_phys;
static struct vm_object pvzone_obj;
static int pv_entry_count=0, pv_entry_max=0, pv_entry_high_water=0;

/*
 * This list exists for the benefit of pmap_map_chunk().  It keeps track
 * of the kernel L2 tables during bootstrap, so that pmap_map_chunk() can
 * find them as necessary.
 *
 * Note that the data on this list MUST remain valid after initarm() returns,
 * as pmap_bootstrap() uses it to contruct L2 table metadata.
 */
SLIST_HEAD(, pv_addr) kernel_pt_list = SLIST_HEAD_INITIALIZER(kernel_pt_list);

static void
pmap_init_l1(struct l1_ttable *l1, pd_entry_t *l1pt)
{
	int i;

	l1->l1_kva = l1pt;
	l1->l1_domain_use_count = 0;
	l1->l1_domain_first = 0;

	for (i = 0; i < PMAP_DOMAINS; i++)
		l1->l1_domain_free[i] = i + 1;

	/*
	 * Copy the kernel's L1 entries to each new L1.
	 */
	if (l1pt != pmap_kernel()->pm_l1->l1_kva)
		memcpy(l1pt, pmap_kernel()->pm_l1->l1_kva, L1_TABLE_SIZE);

	if ((l1->l1_physaddr = pmap_extract(pmap_kernel(), (vm_offset_t)l1pt)) == 0)
		panic("pmap_init_l1: can't get PA of L1 at %p", l1pt);
	SLIST_INSERT_HEAD(&l1_list, l1, l1_link);
	TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);
}

static vm_offset_t
kernel_pt_lookup(vm_paddr_t pa)
{
	struct pv_addr *pv;

	SLIST_FOREACH(pv, &kernel_pt_list, pv_list) {
		if (pv->pv_pa == pa)
			return (pv->pv_va);
	}
	return (0);
}

#if (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0
void
pmap_pte_init_generic(void)
{

	pte_l1_s_cache_mode = L1_S_B|L1_S_C;
	pte_l1_s_cache_mask = L1_S_CACHE_MASK_generic;

	pte_l2_l_cache_mode = L2_B|L2_C;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_generic;

	pte_l2_s_cache_mode = L2_B|L2_C;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_generic;

	/*
	 * If we have a write-through cache, set B and C.  If
	 * we have a write-back cache, then we assume setting
	 * only C will make those pages write-through.
	 */
	if (cpufuncs.cf_dcache_wb_range == (void *) cpufunc_nullop) {
		pte_l1_s_cache_mode_pt = L1_S_B|L1_S_C;
		pte_l2_l_cache_mode_pt = L2_B|L2_C;
		pte_l2_s_cache_mode_pt = L2_B|L2_C;
	} else {
		pte_l1_s_cache_mode_pt = L1_S_C;
		pte_l2_l_cache_mode_pt = L2_C;
		pte_l2_s_cache_mode_pt = L2_C;
	}

	pte_l2_s_prot_u = L2_S_PROT_U_generic;
	pte_l2_s_prot_w = L2_S_PROT_W_generic;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_generic;

	pte_l1_s_proto = L1_S_PROTO_generic;
	pte_l1_c_proto = L1_C_PROTO_generic;
	pte_l2_s_proto = L2_S_PROTO_generic;

	pmap_copy_page_func = pmap_copy_page_generic;
	pmap_zero_page_func = pmap_zero_page_generic;
}

#if defined(CPU_ARM8)
void
pmap_pte_init_arm8(void)
{

	/*
	 * ARM8 is compatible with generic, but we need to use
	 * the page tables uncached.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode_pt = 0;
	pte_l2_l_cache_mode_pt = 0;
	pte_l2_s_cache_mode_pt = 0;
}
#endif /* CPU_ARM8 */

#if defined(CPU_ARM9) && defined(ARM9_CACHE_WRITE_THROUGH)
void
pmap_pte_init_arm9(void)
{

	/*
	 * ARM9 is compatible with generic, but we want to use
	 * write-through caching for now.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode = L1_S_C;
	pte_l2_l_cache_mode = L2_C;
	pte_l2_s_cache_mode = L2_C;

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;
}
#endif /* CPU_ARM9 */
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0 */

#if defined(CPU_ARM10)
void
pmap_pte_init_arm10(void)
{

	/*
	 * ARM10 is compatible with generic, but we want to use
	 * write-through caching for now.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode = L1_S_B | L1_S_C;
	pte_l2_l_cache_mode = L2_B | L2_C;
	pte_l2_s_cache_mode = L2_B | L2_C;

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;

}
#endif /* CPU_ARM10 */

#if  ARM_MMU_SA1 == 1
void
pmap_pte_init_sa1(void)
{

	/*
	 * The StrongARM SA-1 cache does not have a write-through
	 * mode.  So, do the generic initialization, then reset
	 * the page table cache mode to B=1,C=1, and note that
	 * the PTEs need to be sync'd.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode_pt = L1_S_B|L1_S_C;
	pte_l2_l_cache_mode_pt = L2_B|L2_C;
	pte_l2_s_cache_mode_pt = L2_B|L2_C;

	pmap_needs_pte_sync = 1;
}
#endif /* ARM_MMU_SA1 == 1*/

#if ARM_MMU_XSCALE == 1
#if (ARM_NMMUS > 1) || defined (CPU_XSCALE_CORE3)
static u_int xscale_use_minidata;
#endif

void
pmap_pte_init_xscale(void)
{
	uint32_t auxctl;
	int write_through = 0;

	pte_l1_s_cache_mode = L1_S_B|L1_S_C|L1_S_XSCALE_P;
	pte_l1_s_cache_mask = L1_S_CACHE_MASK_xscale;

	pte_l2_l_cache_mode = L2_B|L2_C;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_xscale;

	pte_l2_s_cache_mode = L2_B|L2_C;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_xscale;

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;
#ifdef XSCALE_CACHE_READ_WRITE_ALLOCATE
	/*
	 * The XScale core has an enhanced mode where writes that
	 * miss the cache cause a cache line to be allocated.  This
	 * is significantly faster than the traditional, write-through
	 * behavior of this case.
	 */
	pte_l1_s_cache_mode |= L1_S_XSCALE_TEX(TEX_XSCALE_X);
	pte_l2_l_cache_mode |= L2_XSCALE_L_TEX(TEX_XSCALE_X);
	pte_l2_s_cache_mode |= L2_XSCALE_T_TEX(TEX_XSCALE_X);
#endif /* XSCALE_CACHE_READ_WRITE_ALLOCATE */
#ifdef XSCALE_CACHE_WRITE_THROUGH
	/*
	 * Some versions of the XScale core have various bugs in
	 * their cache units, the work-around for which is to run
	 * the cache in write-through mode.  Unfortunately, this
	 * has a major (negative) impact on performance.  So, we
	 * go ahead and run fast-and-loose, in the hopes that we
	 * don't line up the planets in a way that will trip the
	 * bugs.
	 *
	 * However, we give you the option to be slow-but-correct.
	 */
	write_through = 1;
#elif defined(XSCALE_CACHE_WRITE_BACK)
	/* force write back cache mode */
	write_through = 0;
#elif defined(CPU_XSCALE_PXA2X0)
	/*
	 * Intel PXA2[15]0 processors are known to have a bug in
	 * write-back cache on revision 4 and earlier (stepping
	 * A[01] and B[012]).  Fixed for C0 and later.
	 */
	{
		uint32_t id, type;

		id = cpufunc_id();
		type = id & ~(CPU_ID_XSCALE_COREREV_MASK|CPU_ID_REVISION_MASK);

		if (type == CPU_ID_PXA250 || type == CPU_ID_PXA210) {
			if ((id & CPU_ID_REVISION_MASK) < 5) {
				/* write through for stepping A0-1 and B0-2 */
				write_through = 1;
			}
		}
	}
#endif /* XSCALE_CACHE_WRITE_THROUGH */

	if (write_through) {
		pte_l1_s_cache_mode = L1_S_C;
		pte_l2_l_cache_mode = L2_C;
		pte_l2_s_cache_mode = L2_C;
	}

#if (ARM_NMMUS > 1)
	xscale_use_minidata = 1;
#endif

	pte_l2_s_prot_u = L2_S_PROT_U_xscale;
	pte_l2_s_prot_w = L2_S_PROT_W_xscale;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_xscale;

	pte_l1_s_proto = L1_S_PROTO_xscale;
	pte_l1_c_proto = L1_C_PROTO_xscale;
	pte_l2_s_proto = L2_S_PROTO_xscale;

#ifdef CPU_XSCALE_CORE3
	pmap_copy_page_func = pmap_copy_page_generic;
	pmap_zero_page_func = pmap_zero_page_generic;
	xscale_use_minidata = 0;
	/* Make sure it is L2-cachable */
    	pte_l1_s_cache_mode |= L1_S_XSCALE_TEX(TEX_XSCALE_T);
	pte_l1_s_cache_mode_pt = pte_l1_s_cache_mode &~ L1_S_XSCALE_P;
	pte_l2_l_cache_mode |= L2_XSCALE_L_TEX(TEX_XSCALE_T) ;
	pte_l2_l_cache_mode_pt = pte_l1_s_cache_mode;
	pte_l2_s_cache_mode |= L2_XSCALE_T_TEX(TEX_XSCALE_T);
	pte_l2_s_cache_mode_pt = pte_l2_s_cache_mode;

#else
	pmap_copy_page_func = pmap_copy_page_xscale;
	pmap_zero_page_func = pmap_zero_page_xscale;
#endif

	/*
	 * Disable ECC protection of page table access, for now.
	 */
	__asm __volatile("mrc p15, 0, %0, c1, c0, 1" : "=r" (auxctl));
	auxctl &= ~XSCALE_AUXCTL_P;
	__asm __volatile("mcr p15, 0, %0, c1, c0, 1" : : "r" (auxctl));
}

/*
 * xscale_setup_minidata:
 *
 *	Set up the mini-data cache clean area.  We require the
 *	caller to allocate the right amount of physically and
 *	virtually contiguous space.
 */
extern vm_offset_t xscale_minidata_clean_addr;
extern vm_size_t xscale_minidata_clean_size; /* already initialized */
void
xscale_setup_minidata(vm_offset_t l1pt, vm_offset_t va, vm_paddr_t pa)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pt_entry_t *pte;
	vm_size_t size;
	uint32_t auxctl;

	xscale_minidata_clean_addr = va;

	/* Round it to page size. */
	size = (xscale_minidata_clean_size + L2_S_OFFSET) & L2_S_FRAME;

	for (; size != 0;
	     va += L2_S_SIZE, pa += L2_S_SIZE, size -= L2_S_SIZE) {
		pte = (pt_entry_t *) kernel_pt_lookup(
		    pde[L1_IDX(va)] & L1_C_ADDR_MASK);
		if (pte == NULL)
			panic("xscale_setup_minidata: can't find L2 table for "
			    "VA 0x%08x", (u_int32_t) va);
		pte[l2pte_index(va)] =
		    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, VM_PROT_READ) |
		    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);
	}

	/*
	 * Configure the mini-data cache for write-back with
	 * read/write-allocate.
	 *
	 * NOTE: In order to reconfigure the mini-data cache, we must
	 * make sure it contains no valid data!  In order to do that,
	 * we must issue a global data cache invalidate command!
	 *
	 * WE ASSUME WE ARE RUNNING UN-CACHED WHEN THIS ROUTINE IS CALLED!
	 * THIS IS VERY IMPORTANT!
	 */

	/* Invalidate data and mini-data. */
	__asm __volatile("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));
	__asm __volatile("mrc p15, 0, %0, c1, c0, 1" : "=r" (auxctl));
	auxctl = (auxctl & ~XSCALE_AUXCTL_MD_MASK) | XSCALE_AUXCTL_MD_WB_RWA;
	__asm __volatile("mcr p15, 0, %0, c1, c0, 1" : : "r" (auxctl));
}
#endif

/*
 * Allocate an L1 translation table for the specified pmap.
 * This is called at pmap creation time.
 */
static void
pmap_alloc_l1(pmap_t pm)
{
	struct l1_ttable *l1;
	u_int8_t domain;

	/*
	 * Remove the L1 at the head of the LRU list
	 */
	mtx_lock(&l1_lru_lock);
	l1 = TAILQ_FIRST(&l1_lru_list);
	TAILQ_REMOVE(&l1_lru_list, l1, l1_lru);

	/*
	 * Pick the first available domain number, and update
	 * the link to the next number.
	 */
	domain = l1->l1_domain_first;
	l1->l1_domain_first = l1->l1_domain_free[domain];

	/*
	 * If there are still free domain numbers in this L1,
	 * put it back on the TAIL of the LRU list.
	 */
	if (++l1->l1_domain_use_count < PMAP_DOMAINS)
		TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);

	mtx_unlock(&l1_lru_lock);

	/*
	 * Fix up the relevant bits in the pmap structure
	 */
	pm->pm_l1 = l1;
	pm->pm_domain = domain + 1;
}

/*
 * Free an L1 translation table.
 * This is called at pmap destruction time.
 */
static void
pmap_free_l1(pmap_t pm)
{
	struct l1_ttable *l1 = pm->pm_l1;

	mtx_lock(&l1_lru_lock);

	/*
	 * If this L1 is currently on the LRU list, remove it.
	 */
	if (l1->l1_domain_use_count < PMAP_DOMAINS)
		TAILQ_REMOVE(&l1_lru_list, l1, l1_lru);

	/*
	 * Free up the domain number which was allocated to the pmap
	 */
	l1->l1_domain_free[pm->pm_domain - 1] = l1->l1_domain_first;
	l1->l1_domain_first = pm->pm_domain - 1;
	l1->l1_domain_use_count--;

	/*
	 * The L1 now must have at least 1 free domain, so add
	 * it back to the LRU list. If the use count is zero,
	 * put it at the head of the list, otherwise it goes
	 * to the tail.
	 */
	if (l1->l1_domain_use_count == 0) {
		TAILQ_INSERT_HEAD(&l1_lru_list, l1, l1_lru);
	}	else
		TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);

	mtx_unlock(&l1_lru_lock);
}

static PMAP_INLINE void
pmap_use_l1(pmap_t pm)
{
	struct l1_ttable *l1;

	/*
	 * Do nothing if we're in interrupt context.
	 * Access to an L1 by the kernel pmap must not affect
	 * the LRU list.
	 */
	if (pm == pmap_kernel())
		return;

	l1 = pm->pm_l1;

	/*
	 * If the L1 is not currently on the LRU list, just return
	 */
	if (l1->l1_domain_use_count == PMAP_DOMAINS)
		return;

	mtx_lock(&l1_lru_lock);

	/*
	 * Check the use count again, now that we've acquired the lock
	 */
	if (l1->l1_domain_use_count == PMAP_DOMAINS) {
		mtx_unlock(&l1_lru_lock);
		return;
	}

	/*
	 * Move the L1 to the back of the LRU list
	 */
	TAILQ_REMOVE(&l1_lru_list, l1, l1_lru);
	TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);

	mtx_unlock(&l1_lru_lock);
}


/*
 * Returns a pointer to the L2 bucket associated with the specified pmap
 * and VA, or NULL if no L2 bucket exists for the address.
 */
static PMAP_INLINE struct l2_bucket *
pmap_get_l2_bucket(pmap_t pm, vm_offset_t va)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	u_short l1idx;

	l1idx = L1_IDX(va);

	if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL ||
	    (l2b = &l2->l2_bucket[L2_BUCKET(l1idx)])->l2b_kva == NULL)
		return (NULL);

	return (l2b);
}

/*
 * Returns a pointer to the L2 bucket associated with the specified pmap
 * and VA.
 *
 * If no L2 bucket exists, perform the necessary allocations to put an L2
 * bucket/page table in place.
 *
 * Note that if a new L2 bucket/page was allocated, the caller *must*
 * increment the bucket occupancy counter appropriately *before* 
 * releasing the pmap's lock to ensure no other thread or cpu deallocates
 * the bucket/page in the meantime.
 */
static struct l2_bucket *
pmap_alloc_l2_bucket(pmap_t pm, vm_offset_t va)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	u_short l1idx;

	l1idx = L1_IDX(va);

	PMAP_ASSERT_LOCKED(pm);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL) {
		/*
		 * No mapping at this address, as there is
		 * no entry in the L1 table.
		 * Need to allocate a new l2_dtable.
		 */
again_l2table:
		PMAP_UNLOCK(pm);
		vm_page_unlock_queues();
		if ((l2 = pmap_alloc_l2_dtable()) == NULL) {
			vm_page_lock_queues();
			PMAP_LOCK(pm);
			return (NULL);
		}
		vm_page_lock_queues();
		PMAP_LOCK(pm);
		if (pm->pm_l2[L2_IDX(l1idx)] != NULL) {
			PMAP_UNLOCK(pm);
			vm_page_unlock_queues();
			uma_zfree(l2table_zone, l2);
			vm_page_lock_queues();
			PMAP_LOCK(pm);
			l2 = pm->pm_l2[L2_IDX(l1idx)];
			if (l2 == NULL)
				goto again_l2table;
			/*
			 * Someone already allocated the l2_dtable while
			 * we were doing the same.
			 */
		} else {
			bzero(l2, sizeof(*l2));
			/*
			 * Link it into the parent pmap
			 */
			pm->pm_l2[L2_IDX(l1idx)] = l2;
		}
	} 

	l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];

	/*
	 * Fetch pointer to the L2 page table associated with the address.
	 */
	if (l2b->l2b_kva == NULL) {
		pt_entry_t *ptep;

		/*
		 * No L2 page table has been allocated. Chances are, this
		 * is because we just allocated the l2_dtable, above.
		 */
again_ptep:
		PMAP_UNLOCK(pm);
		vm_page_unlock_queues();
		ptep = (void*)uma_zalloc(l2zone, M_NOWAIT|M_USE_RESERVE);
		vm_page_lock_queues();
		PMAP_LOCK(pm);
		if (l2b->l2b_kva != 0) {
			/* We lost the race. */
			PMAP_UNLOCK(pm);
			vm_page_unlock_queues();
			uma_zfree(l2zone, ptep);
			vm_page_lock_queues();
			PMAP_LOCK(pm);
			if (l2b->l2b_kva == 0)
				goto again_ptep;
			return (l2b);
		}
		l2b->l2b_phys = vtophys(ptep);
		if (ptep == NULL) {
			/*
			 * Oops, no more L2 page tables available at this
			 * time. We may need to deallocate the l2_dtable
			 * if we allocated a new one above.
			 */
			if (l2->l2_occupancy == 0) {
				pm->pm_l2[L2_IDX(l1idx)] = NULL;
				pmap_free_l2_dtable(l2);
			}
			return (NULL);
		}

		l2->l2_occupancy++;
		l2b->l2b_kva = ptep;
		l2b->l2b_l1idx = l1idx;
	}

	return (l2b);
}

static PMAP_INLINE void
#ifndef PMAP_INCLUDE_PTE_SYNC
pmap_free_l2_ptp(pt_entry_t *l2)
#else
pmap_free_l2_ptp(boolean_t need_sync, pt_entry_t *l2)
#endif
{
#ifdef PMAP_INCLUDE_PTE_SYNC
	/*
	 * Note: With a write-back cache, we may need to sync this
	 * L2 table before re-using it.
	 * This is because it may have belonged to a non-current
	 * pmap, in which case the cache syncs would have been
	 * skipped when the pages were being unmapped. If the
	 * L2 table were then to be immediately re-allocated to
	 * the *current* pmap, it may well contain stale mappings
	 * which have not yet been cleared by a cache write-back
	 * and so would still be visible to the mmu.
	 */
	if (need_sync)
		PTE_SYNC_RANGE(l2, L2_TABLE_SIZE_REAL / sizeof(pt_entry_t));
#endif
	uma_zfree(l2zone, l2);
}
/*
 * One or more mappings in the specified L2 descriptor table have just been
 * invalidated.
 *
 * Garbage collect the metadata and descriptor table itself if necessary.
 *
 * The pmap lock must be acquired when this is called (not necessary
 * for the kernel pmap).
 */
static void
pmap_free_l2_bucket(pmap_t pm, struct l2_bucket *l2b, u_int count)
{
	struct l2_dtable *l2;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep;
	u_short l1idx;


	/*
	 * Update the bucket's reference count according to how many
	 * PTEs the caller has just invalidated.
	 */
	l2b->l2b_occupancy -= count;

	/*
	 * Note:
	 *
	 * Level 2 page tables allocated to the kernel pmap are never freed
	 * as that would require checking all Level 1 page tables and
	 * removing any references to the Level 2 page table. See also the
	 * comment elsewhere about never freeing bootstrap L2 descriptors.
	 *
	 * We make do with just invalidating the mapping in the L2 table.
	 *
	 * This isn't really a big deal in practice and, in fact, leads
	 * to a performance win over time as we don't need to continually
	 * alloc/free.
	 */
	if (l2b->l2b_occupancy > 0 || pm == pmap_kernel())
		return;

	/*
	 * There are no more valid mappings in this level 2 page table.
	 * Go ahead and NULL-out the pointer in the bucket, then
	 * free the page table.
	 */
	l1idx = l2b->l2b_l1idx;
	ptep = l2b->l2b_kva;
	l2b->l2b_kva = NULL;

	pl1pd = &pm->pm_l1->l1_kva[l1idx];

	/*
	 * If the L1 slot matches the pmap's domain
	 * number, then invalidate it.
	 */
	l1pd = *pl1pd & (L1_TYPE_MASK | L1_C_DOM_MASK);
	if (l1pd == (L1_C_DOM(pm->pm_domain) | L1_TYPE_C)) {
		*pl1pd = 0;
		PTE_SYNC(pl1pd);
	}

	/*
	 * Release the L2 descriptor table back to the pool cache.
	 */
#ifndef PMAP_INCLUDE_PTE_SYNC
	pmap_free_l2_ptp(ptep);
#else
	pmap_free_l2_ptp(!pmap_is_current(pm), ptep);
#endif

	/*
	 * Update the reference count in the associated l2_dtable
	 */
	l2 = pm->pm_l2[L2_IDX(l1idx)];
	if (--l2->l2_occupancy > 0)
		return;

	/*
	 * There are no more valid mappings in any of the Level 1
	 * slots managed by this l2_dtable. Go ahead and NULL-out
	 * the pointer in the parent pmap and free the l2_dtable.
	 */
	pm->pm_l2[L2_IDX(l1idx)] = NULL;
	pmap_free_l2_dtable(l2);
}

/*
 * Pool cache constructors for L2 descriptor tables, metadata and pmap
 * structures.
 */
static int
pmap_l2ptp_ctor(void *mem, int size, void *arg, int flags)
{
#ifndef PMAP_INCLUDE_PTE_SYNC
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
#ifdef ARM_USE_SMALL_ALLOC
	pd_entry_t *pde;
#endif
	vm_offset_t va = (vm_offset_t)mem & ~PAGE_MASK;

	/*
	 * The mappings for these page tables were initially made using
	 * pmap_kenter() by the pool subsystem. Therefore, the cache-
	 * mode will not be right for page table mappings. To avoid
	 * polluting the pmap_kenter() code with a special case for
	 * page tables, we simply fix up the cache-mode here if it's not
	 * correct.
	 */
#ifdef ARM_USE_SMALL_ALLOC
	pde = &kernel_pmap->pm_l1->l1_kva[L1_IDX(va)];
	if (!l1pte_section_p(*pde)) {
#endif
		l2b = pmap_get_l2_bucket(pmap_kernel(), va);
		ptep = &l2b->l2b_kva[l2pte_index(va)];
		pte = *ptep;
		
		if ((pte & L2_S_CACHE_MASK) != pte_l2_s_cache_mode_pt) {
			/*
			 * Page tables must have the cache-mode set to 
			 * Write-Thru.
			 */
			*ptep = (pte & ~L2_S_CACHE_MASK) | pte_l2_s_cache_mode_pt;
			PTE_SYNC(ptep);
			cpu_tlb_flushD_SE(va);
			cpu_cpwait();
		}
#ifdef ARM_USE_SMALL_ALLOC
	}
#endif
#endif
	memset(mem, 0, L2_TABLE_SIZE_REAL);
	PTE_SYNC_RANGE(mem, L2_TABLE_SIZE_REAL / sizeof(pt_entry_t));
	return (0);
}

/*
 * A bunch of routines to conditionally flush the caches/TLB depending
 * on whether the specified pmap actually needs to be flushed at any
 * given time.
 */
static PMAP_INLINE void
pmap_tlb_flushID_SE(pmap_t pm, vm_offset_t va)
{

	if (pmap_is_current(pm))
		cpu_tlb_flushID_SE(va);
}

static PMAP_INLINE void
pmap_tlb_flushD_SE(pmap_t pm, vm_offset_t va)
{

	if (pmap_is_current(pm))
		cpu_tlb_flushD_SE(va);
}

static PMAP_INLINE void
pmap_tlb_flushID(pmap_t pm)
{

	if (pmap_is_current(pm))
		cpu_tlb_flushID();
}
static PMAP_INLINE void
pmap_tlb_flushD(pmap_t pm)
{

	if (pmap_is_current(pm))
		cpu_tlb_flushD();
}

static PMAP_INLINE void
pmap_idcache_wbinv_range(pmap_t pm, vm_offset_t va, vm_size_t len)
{

	if (pmap_is_current(pm))
		cpu_idcache_wbinv_range(va, len);
}

static PMAP_INLINE void
pmap_dcache_wb_range(pmap_t pm, vm_offset_t va, vm_size_t len,
    boolean_t do_inv, boolean_t rd_only)
{

	if (pmap_is_current(pm)) {
		if (do_inv) {
			if (rd_only)
				cpu_dcache_inv_range(va, len);
			else
				cpu_dcache_wbinv_range(va, len);
		} else
		if (!rd_only)
			cpu_dcache_wb_range(va, len);
	}
}

static PMAP_INLINE void
pmap_idcache_wbinv_all(pmap_t pm)
{

	if (pmap_is_current(pm))
		cpu_idcache_wbinv_all();
}

static PMAP_INLINE void
pmap_dcache_wbinv_all(pmap_t pm)
{

	if (pmap_is_current(pm))
		cpu_dcache_wbinv_all();
}

/*
 * PTE_SYNC_CURRENT:
 *
 *     Make sure the pte is written out to RAM.
 *     We need to do this for one of two cases:
 *       - We're dealing with the kernel pmap
 *       - There is no pmap active in the cache/tlb.
 *       - The specified pmap is 'active' in the cache/tlb.
 */
#ifdef PMAP_INCLUDE_PTE_SYNC
#define	PTE_SYNC_CURRENT(pm, ptep)	\
do {					\
	if (PMAP_NEEDS_PTE_SYNC && 	\
	    pmap_is_current(pm))	\
		PTE_SYNC(ptep);		\
} while (/*CONSTCOND*/0)
#else
#define	PTE_SYNC_CURRENT(pm, ptep)	/* nothing */
#endif

/*
 * Since we have a virtually indexed cache, we may need to inhibit caching if
 * there is more than one mapping and at least one of them is writable.
 * Since we purge the cache on every context switch, we only need to check for
 * other mappings within the same pmap, or kernel_pmap.
 * This function is also called when a page is unmapped, to possibly reenable
 * caching on any remaining mappings.
 *
 * The code implements the following logic, where:
 *
 * KW = # of kernel read/write pages
 * KR = # of kernel read only pages
 * UW = # of user read/write pages
 * UR = # of user read only pages
 * 
 * KC = kernel mapping is cacheable
 * UC = user mapping is cacheable
 *
 *               KW=0,KR=0  KW=0,KR>0  KW=1,KR=0  KW>1,KR>=0
 *             +---------------------------------------------
 * UW=0,UR=0   | ---        KC=1       KC=1       KC=0
 * UW=0,UR>0   | UC=1       KC=1,UC=1  KC=0,UC=0  KC=0,UC=0
 * UW=1,UR=0   | UC=1       KC=0,UC=0  KC=0,UC=0  KC=0,UC=0
 * UW>1,UR>=0  | UC=0       KC=0,UC=0  KC=0,UC=0  KC=0,UC=0
 */

static const int pmap_vac_flags[4][4] = {
	{-1,		0,		0,		PVF_KNC},
	{0,		0,		PVF_NC,		PVF_NC},
	{0,		PVF_NC,		PVF_NC,		PVF_NC},
	{PVF_UNC,	PVF_NC,		PVF_NC,		PVF_NC}
};

static PMAP_INLINE int
pmap_get_vac_flags(const struct vm_page *pg)
{
	int kidx, uidx;

	kidx = 0;
	if (pg->md.kro_mappings || pg->md.krw_mappings > 1)
		kidx |= 1;
	if (pg->md.krw_mappings)
		kidx |= 2;

	uidx = 0;
	if (pg->md.uro_mappings || pg->md.urw_mappings > 1)
		uidx |= 1;
	if (pg->md.urw_mappings)
		uidx |= 2;

	return (pmap_vac_flags[uidx][kidx]);
}

static __inline void
pmap_vac_me_harder(struct vm_page *pg, pmap_t pm, vm_offset_t va)
{
	int nattr;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	nattr = pmap_get_vac_flags(pg);

	if (nattr < 0) {
		pg->md.pvh_attrs &= ~PVF_NC;
		return;
	}

	if (nattr == 0 && (pg->md.pvh_attrs & PVF_NC) == 0) {
		return;
	}

	if (pm == pmap_kernel())
		pmap_vac_me_kpmap(pg, pm, va);
	else
		pmap_vac_me_user(pg, pm, va);

	pg->md.pvh_attrs = (pg->md.pvh_attrs & ~PVF_NC) | nattr;
}

static void
pmap_vac_me_kpmap(struct vm_page *pg, pmap_t pm, vm_offset_t va)
{
	u_int u_cacheable, u_entries;
	struct pv_entry *pv;
	pmap_t last_pmap = pm;

	/* 
	 * Pass one, see if there are both kernel and user pmaps for
	 * this page.  Calculate whether there are user-writable or
	 * kernel-writable pages.
	 */
	u_cacheable = 0;
	TAILQ_FOREACH(pv, &pg->md.pv_list, pv_list) {
		if (pv->pv_pmap != pm && (pv->pv_flags & PVF_NC) == 0)
			u_cacheable++;
	}

	u_entries = pg->md.urw_mappings + pg->md.uro_mappings;

	/* 
	 * We know we have just been updating a kernel entry, so if
	 * all user pages are already cacheable, then there is nothing
	 * further to do.
	 */
	if (pg->md.k_mappings == 0 && u_cacheable == u_entries)
		return;

	if (u_entries) {
		/* 
		 * Scan over the list again, for each entry, if it
		 * might not be set correctly, call pmap_vac_me_user
		 * to recalculate the settings.
		 */
		TAILQ_FOREACH(pv, &pg->md.pv_list, pv_list) {
			/* 
			 * We know kernel mappings will get set
			 * correctly in other calls.  We also know
			 * that if the pmap is the same as last_pmap
			 * then we've just handled this entry.
			 */
			if (pv->pv_pmap == pm || pv->pv_pmap == last_pmap)
				continue;

			/* 
			 * If there are kernel entries and this page
			 * is writable but non-cacheable, then we can
			 * skip this entry also.  
			 */
			if (pg->md.k_mappings &&
			    (pv->pv_flags & (PVF_NC | PVF_WRITE)) ==
			    (PVF_NC | PVF_WRITE))
				continue;

			/* 
			 * Similarly if there are no kernel-writable 
			 * entries and the page is already 
			 * read-only/cacheable.
			 */
			if (pg->md.krw_mappings == 0 &&
			    (pv->pv_flags & (PVF_NC | PVF_WRITE)) == 0)
				continue;

			/* 
			 * For some of the remaining cases, we know
			 * that we must recalculate, but for others we
			 * can't tell if they are correct or not, so
			 * we recalculate anyway.
			 */
			pmap_vac_me_user(pg, (last_pmap = pv->pv_pmap), 0);
		}

		if (pg->md.k_mappings == 0)
			return;
	}

	pmap_vac_me_user(pg, pm, va);
}

static void
pmap_vac_me_user(struct vm_page *pg, pmap_t pm, vm_offset_t va)
{
	pmap_t kpmap = pmap_kernel();
	struct pv_entry *pv, *npv;
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	u_int entries = 0;
	u_int writable = 0;
	u_int cacheable_entries = 0;
	u_int kern_cacheable = 0;
	u_int other_writable = 0;

	/*
	 * Count mappings and writable mappings in this pmap.
	 * Include kernel mappings as part of our own.
	 * Keep a pointer to the first one.
	 */
	npv = TAILQ_FIRST(&pg->md.pv_list);
	TAILQ_FOREACH(pv, &pg->md.pv_list, pv_list) {
		/* Count mappings in the same pmap */
		if (pm == pv->pv_pmap || kpmap == pv->pv_pmap) {
			if (entries++ == 0)
				npv = pv;

			/* Cacheable mappings */
			if ((pv->pv_flags & PVF_NC) == 0) {
				cacheable_entries++;
				if (kpmap == pv->pv_pmap)
					kern_cacheable++;
			}

			/* Writable mappings */
			if (pv->pv_flags & PVF_WRITE)
				++writable;
		} else
		if (pv->pv_flags & PVF_WRITE)
			other_writable = 1;
	}

	/*
	 * Enable or disable caching as necessary.
	 * Note: the first entry might be part of the kernel pmap,
	 * so we can't assume this is indicative of the state of the
	 * other (maybe non-kpmap) entries.
	 */
	if ((entries > 1 && writable) ||
	    (entries > 0 && pm == kpmap && other_writable)) {
		if (cacheable_entries == 0)
			return;

		for (pv = npv; pv; pv = TAILQ_NEXT(pv, pv_list)) {
			if ((pm != pv->pv_pmap && kpmap != pv->pv_pmap) ||
			    (pv->pv_flags & PVF_NC))
				continue;

			pv->pv_flags |= PVF_NC;

			l2b = pmap_get_l2_bucket(pv->pv_pmap, pv->pv_va);
			ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
			pte = *ptep & ~L2_S_CACHE_MASK;

			if ((va != pv->pv_va || pm != pv->pv_pmap) &&
			    l2pte_valid(pte)) {
				if (PV_BEEN_EXECD(pv->pv_flags)) {
					pmap_idcache_wbinv_range(pv->pv_pmap,
					    pv->pv_va, PAGE_SIZE);
					pmap_tlb_flushID_SE(pv->pv_pmap,
					    pv->pv_va);
				} else
				if (PV_BEEN_REFD(pv->pv_flags)) {
					pmap_dcache_wb_range(pv->pv_pmap,
					    pv->pv_va, PAGE_SIZE, TRUE,
					    (pv->pv_flags & PVF_WRITE) == 0);
					pmap_tlb_flushD_SE(pv->pv_pmap,
					    pv->pv_va);
				}
			}

			*ptep = pte;
			PTE_SYNC_CURRENT(pv->pv_pmap, ptep);
		}
		cpu_cpwait();
	} else
	if (entries > cacheable_entries) {
		/*
		 * Turn cacheing back on for some pages.  If it is a kernel
		 * page, only do so if there are no other writable pages.
		 */
		for (pv = npv; pv; pv = TAILQ_NEXT(pv, pv_list)) {
			if (!(pv->pv_flags & PVF_NC) || (pm != pv->pv_pmap &&
			    (kpmap != pv->pv_pmap || other_writable)))
				continue;

			pv->pv_flags &= ~PVF_NC;

			l2b = pmap_get_l2_bucket(pv->pv_pmap, pv->pv_va);
			ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
			pte = (*ptep & ~L2_S_CACHE_MASK) | pte_l2_s_cache_mode;

			if (l2pte_valid(pte)) {
				if (PV_BEEN_EXECD(pv->pv_flags)) {
					pmap_tlb_flushID_SE(pv->pv_pmap,
					    pv->pv_va);
				} else
				if (PV_BEEN_REFD(pv->pv_flags)) {
					pmap_tlb_flushD_SE(pv->pv_pmap,
					    pv->pv_va);
				}
			}

			*ptep = pte;
			PTE_SYNC_CURRENT(pv->pv_pmap, ptep);
		}
	}
}

/*
 * Modify pte bits for all ptes corresponding to the given physical address.
 * We use `maskbits' rather than `clearbits' because we're always passing
 * constants and the latter would require an extra inversion at run-time.
 */
static int 
pmap_clearbit(struct vm_page *pg, u_int maskbits)
{
	struct l2_bucket *l2b;
	struct pv_entry *pv;
	pt_entry_t *ptep, npte, opte;
	pmap_t pm;
	vm_offset_t va;
	u_int oflags;
	int count = 0;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);

	/*
	 * Clear saved attributes (modify, reference)
	 */
	pg->md.pvh_attrs &= ~(maskbits & (PVF_MOD | PVF_REF));

	if (TAILQ_EMPTY(&pg->md.pv_list)) {
		return (0);
	}

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 */
	TAILQ_FOREACH(pv, &pg->md.pv_list, pv_list) {
		va = pv->pv_va;
		pm = pv->pv_pmap;
		oflags = pv->pv_flags;
		pv->pv_flags &= ~maskbits;

		PMAP_LOCK(pm);

		l2b = pmap_get_l2_bucket(pm, va);

		ptep = &l2b->l2b_kva[l2pte_index(va)];
		npte = opte = *ptep;

		if (maskbits & (PVF_WRITE|PVF_MOD)) {
			if ((pv->pv_flags & PVF_NC)) {
				/* 
				 * Entry is not cacheable:
				 *
				 * Don't turn caching on again if this is a 
				 * modified emulation. This would be
				 * inconsitent with the settings created by
				 * pmap_vac_me_harder(). Otherwise, it's safe
				 * to re-enable cacheing.
				 *
				 * There's no need to call pmap_vac_me_harder()
				 * here: all pages are losing their write
				 * permission.
				 */
				if (maskbits & PVF_WRITE) {
					npte |= pte_l2_s_cache_mode;
					pv->pv_flags &= ~PVF_NC;
				}
			} else
			if (opte & L2_S_PROT_W) {
				vm_page_dirty(pg);
				/* 
				 * Entry is writable/cacheable: check if pmap
				 * is current if it is flush it, otherwise it
				 * won't be in the cache
				 */
				if (PV_BEEN_EXECD(oflags))
					pmap_idcache_wbinv_range(pm, pv->pv_va,
					    PAGE_SIZE);
				else
				if (PV_BEEN_REFD(oflags))
					pmap_dcache_wb_range(pm, pv->pv_va,
					    PAGE_SIZE,
					    (maskbits & PVF_REF) ? TRUE : FALSE,
					    FALSE);
			}

			/* make the pte read only */
			npte &= ~L2_S_PROT_W;

			if (maskbits & PVF_WRITE) {
				/*
				 * Keep alias accounting up to date
				 */
				if (pv->pv_pmap == pmap_kernel()) {
					if (oflags & PVF_WRITE) {
						pg->md.krw_mappings--;
						pg->md.kro_mappings++;
					}
				} else
				if (oflags & PVF_WRITE) {
					pg->md.urw_mappings--;
					pg->md.uro_mappings++;
				}
			}
		}

		if (maskbits & PVF_REF) {
			if ((pv->pv_flags & PVF_NC) == 0 &&
			    (maskbits & (PVF_WRITE|PVF_MOD)) == 0) {
				/*
				 * Check npte here; we may have already
				 * done the wbinv above, and the validity
				 * of the PTE is the same for opte and
				 * npte.
				 */
				if (npte & L2_S_PROT_W) {
					if (PV_BEEN_EXECD(oflags))
						pmap_idcache_wbinv_range(pm,
						    pv->pv_va, PAGE_SIZE);
					else
					if (PV_BEEN_REFD(oflags))
						pmap_dcache_wb_range(pm,
						    pv->pv_va, PAGE_SIZE,
						    TRUE, FALSE);
				} else
				if ((npte & L2_TYPE_MASK) != L2_TYPE_INV) {
					/* XXXJRT need idcache_inv_range */
					if (PV_BEEN_EXECD(oflags))
						pmap_idcache_wbinv_range(pm,
						    pv->pv_va, PAGE_SIZE);
					else
					if (PV_BEEN_REFD(oflags))
						pmap_dcache_wb_range(pm,
						    pv->pv_va, PAGE_SIZE,
						    TRUE, TRUE);
				}
			}

			/*
			 * Make the PTE invalid so that we will take a
			 * page fault the next time the mapping is
			 * referenced.
			 */
			npte &= ~L2_TYPE_MASK;
			npte |= L2_TYPE_INV;
		}

		if (npte != opte) {
			count++;
			*ptep = npte;
			PTE_SYNC(ptep);
			/* Flush the TLB entry if a current pmap. */
			if (PV_BEEN_EXECD(oflags))
				pmap_tlb_flushID_SE(pm, pv->pv_va);
			else
			if (PV_BEEN_REFD(oflags))
				pmap_tlb_flushD_SE(pm, pv->pv_va);
		}

		PMAP_UNLOCK(pm);

	}

	if (maskbits & PVF_WRITE)
		vm_page_flag_clear(pg, PG_WRITEABLE);
	return (count);
}

/*
 * main pv_entry manipulation functions:
 *   pmap_enter_pv: enter a mapping onto a vm_page list
 *   pmap_remove_pv: remove a mappiing from a vm_page list
 *
 * NOTE: pmap_enter_pv expects to lock the pvh itself
 *       pmap_remove_pv expects te caller to lock the pvh before calling
 */

/*
 * pmap_enter_pv: enter a mapping onto a vm_page lst
 *
 * => caller should hold the proper lock on pmap_main_lock
 * => caller should have pmap locked
 * => we will gain the lock on the vm_page and allocate the new pv_entry
 * => caller should adjust ptp's wire_count before calling
 * => caller should not adjust pmap's wire_count
 */
static void
pmap_enter_pv(struct vm_page *pg, struct pv_entry *pve, pmap_t pm,
    vm_offset_t va, u_int flags)
{

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	PMAP_ASSERT_LOCKED(pm);
	pve->pv_pmap = pm;
	pve->pv_va = va;
	pve->pv_flags = flags;

	TAILQ_INSERT_HEAD(&pg->md.pv_list, pve, pv_list);
	TAILQ_INSERT_HEAD(&pm->pm_pvlist, pve, pv_plist);
	pg->md.pvh_attrs |= flags & (PVF_REF | PVF_MOD);
	if (pm == pmap_kernel()) {
		if (flags & PVF_WRITE)
			pg->md.krw_mappings++;
		else
			pg->md.kro_mappings++;
	} else {
		if (flags & PVF_WRITE)
			pg->md.urw_mappings++;
		else
			pg->md.uro_mappings++;
	}
	pg->md.pv_list_count++;
	if (pve->pv_flags & PVF_WIRED)
		++pm->pm_stats.wired_count;
	vm_page_flag_set(pg, PG_REFERENCED);
}

/*
 *
 * pmap_find_pv: Find a pv entry
 *
 * => caller should hold lock on vm_page
 */
static PMAP_INLINE struct pv_entry *
pmap_find_pv(struct vm_page *pg, pmap_t pm, vm_offset_t va)
{
	struct pv_entry *pv;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	TAILQ_FOREACH(pv, &pg->md.pv_list, pv_list)
	    if (pm == pv->pv_pmap && va == pv->pv_va)
		    break;
	return (pv);
}

/*
 * vector_page_setprot:
 *
 *	Manipulate the protection of the vector page.
 */
void
vector_page_setprot(int prot)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep;

	l2b = pmap_get_l2_bucket(pmap_kernel(), vector_page);

	ptep = &l2b->l2b_kva[l2pte_index(vector_page)];

	*ptep = (*ptep & ~L1_S_PROT_MASK) | L2_S_PROT(PTE_KERNEL, prot);
	PTE_SYNC(ptep);
	cpu_tlb_flushD_SE(vector_page);
	cpu_cpwait();
}

/*
 * pmap_remove_pv: try to remove a mapping from a pv_list
 *
 * => caller should hold proper lock on pmap_main_lock
 * => pmap should be locked
 * => caller should hold lock on vm_page [so that attrs can be adjusted]
 * => caller should adjust ptp's wire_count and free PTP if needed
 * => caller should NOT adjust pmap's wire_count
 * => we return the removed pve
 */

static void
pmap_nuke_pv(struct vm_page *pg, pmap_t pm, struct pv_entry *pve)
{

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	PMAP_ASSERT_LOCKED(pm);
	TAILQ_REMOVE(&pg->md.pv_list, pve, pv_list);
	TAILQ_REMOVE(&pm->pm_pvlist, pve, pv_plist);
	if (pve->pv_flags & PVF_WIRED)
		--pm->pm_stats.wired_count;
	pg->md.pv_list_count--;
	if (pg->md.pvh_attrs & PVF_MOD)
		vm_page_dirty(pg);
	if (pm == pmap_kernel()) {
		if (pve->pv_flags & PVF_WRITE)
			pg->md.krw_mappings--;
		else
			pg->md.kro_mappings--;
	} else
		if (pve->pv_flags & PVF_WRITE)
			pg->md.urw_mappings--;
		else
			pg->md.uro_mappings--;
	if (TAILQ_FIRST(&pg->md.pv_list) == NULL ||
	    (pg->md.krw_mappings == 0 && pg->md.urw_mappings == 0)) {
		pg->md.pvh_attrs &= ~PVF_MOD;
		if (TAILQ_FIRST(&pg->md.pv_list) == NULL)
			pg->md.pvh_attrs &= ~PVF_REF;
		vm_page_flag_clear(pg, PG_WRITEABLE);
	}
	if (TAILQ_FIRST(&pg->md.pv_list))
		vm_page_flag_set(pg, PG_REFERENCED);
	if (pve->pv_flags & PVF_WRITE)
		pmap_vac_me_harder(pg, pm, 0);
}

static struct pv_entry *
pmap_remove_pv(struct vm_page *pg, pmap_t pm, vm_offset_t va)
{
	struct pv_entry *pve;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	pve = TAILQ_FIRST(&pg->md.pv_list);

	while (pve) {
		if (pve->pv_pmap == pm && pve->pv_va == va) {	/* match? */
			pmap_nuke_pv(pg, pm, pve);
			break;
		}
		pve = TAILQ_NEXT(pve, pv_list);
	}

	return(pve);				/* return removed pve */
}
/*
 *
 * pmap_modify_pv: Update pv flags
 *
 * => caller should hold lock on vm_page [so that attrs can be adjusted]
 * => caller should NOT adjust pmap's wire_count
 * => caller must call pmap_vac_me_harder() if writable status of a page
 *    may have changed.
 * => we return the old flags
 * 
 * Modify a physical-virtual mapping in the pv table
 */
static u_int
pmap_modify_pv(struct vm_page *pg, pmap_t pm, vm_offset_t va,
    u_int clr_mask, u_int set_mask)
{
	struct pv_entry *npv;
	u_int flags, oflags;

	PMAP_ASSERT_LOCKED(pm);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((npv = pmap_find_pv(pg, pm, va)) == NULL)
		return (0);

	/*
	 * There is at least one VA mapping this page.
	 */

	if (clr_mask & (PVF_REF | PVF_MOD))
		pg->md.pvh_attrs |= set_mask & (PVF_REF | PVF_MOD);

	oflags = npv->pv_flags;
	npv->pv_flags = flags = (oflags & ~clr_mask) | set_mask;

	if ((flags ^ oflags) & PVF_WIRED) {
		if (flags & PVF_WIRED)
			++pm->pm_stats.wired_count;
		else
			--pm->pm_stats.wired_count;
	}

	if ((flags ^ oflags) & PVF_WRITE) {
		if (pm == pmap_kernel()) {
			if (flags & PVF_WRITE) {
				pg->md.krw_mappings++;
				pg->md.kro_mappings--;
			} else {
				pg->md.kro_mappings++;
				pg->md.krw_mappings--;
			}
		} else
		if (flags & PVF_WRITE) {
			pg->md.urw_mappings++;
			pg->md.uro_mappings--;
		} else {
			pg->md.uro_mappings++;
			pg->md.urw_mappings--;
		}
		if (pg->md.krw_mappings == 0 && pg->md.urw_mappings == 0) {
			pg->md.pvh_attrs &= ~PVF_MOD;
			vm_page_flag_clear(pg, PG_WRITEABLE);
		}
		pmap_vac_me_harder(pg, pm, 0);
	}

	return (oflags);
}

/* Function to set the debug level of the pmap code */
#ifdef PMAP_DEBUG
void
pmap_debug(int level)
{
	pmap_debug_level = level;
	dprintf("pmap_debug: level=%d\n", pmap_debug_level);
}
#endif  /* PMAP_DEBUG */

void
pmap_pinit0(struct pmap *pmap)
{
	PDEBUG(1, printf("pmap_pinit0: pmap = %08x\n", (u_int32_t) pmap));

	dprintf("pmap_pinit0: pmap = %08x, pm_pdir = %08x\n",
		(u_int32_t) pmap, (u_int32_t) pmap->pm_pdir);
	bcopy(kernel_pmap, pmap, sizeof(*pmap));
	bzero(&pmap->pm_mtx, sizeof(pmap->pm_mtx));
	PMAP_LOCK_INIT(pmap);
}

/*
 *	Initialize a vm_page's machine-dependent fields.
 */
void
pmap_page_init(vm_page_t m)
{

	TAILQ_INIT(&m->md.pv_list);
	m->md.pv_list_count = 0;
}

/*
 *      Initialize the pmap module.
 *      Called by vm_init, to initialize any structures that the pmap
 *      system needs to map virtual memory.
 */
void
pmap_init(void)
{
	int shpgperproc = PMAP_SHPGPERPROC;

	PDEBUG(1, printf("pmap_init: phys_start = %08x\n"));

	/*
	 * init the pv free list
	 */
	pvzone = uma_zcreate("PV ENTRY", sizeof (struct pv_entry), NULL, NULL, 
	    NULL, NULL, UMA_ALIGN_PTR, UMA_ZONE_VM | UMA_ZONE_NOFREE);
	/*
	 * Now it is safe to enable pv_table recording.
	 */
	PDEBUG(1, printf("pmap_init: done!\n"));

	TUNABLE_INT_FETCH("vm.pmap.shpgperproc", &shpgperproc);
	
	pv_entry_max = shpgperproc * maxproc + cnt.v_page_count;
	pv_entry_high_water = 9 * (pv_entry_max / 10);
	l2zone = uma_zcreate("L2 Table", L2_TABLE_SIZE_REAL, pmap_l2ptp_ctor,
	    NULL, NULL, NULL, UMA_ALIGN_PTR, UMA_ZONE_VM | UMA_ZONE_NOFREE);
	l2table_zone = uma_zcreate("L2 Table", sizeof(struct l2_dtable),
	    NULL, NULL, NULL, NULL, UMA_ALIGN_PTR,
	    UMA_ZONE_VM | UMA_ZONE_NOFREE);

	uma_zone_set_obj(pvzone, &pvzone_obj, pv_entry_max);

}

int
pmap_fault_fixup(pmap_t pm, vm_offset_t va, vm_prot_t ftype, int user)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep, pte;
	vm_paddr_t pa;
	u_int l1idx;
	int rv = 0;

	l1idx = L1_IDX(va);
	vm_page_lock_queues();
	PMAP_LOCK(pm);

	/*
	 * If there is no l2_dtable for this address, then the process
	 * has no business accessing it.
	 *
	 * Note: This will catch userland processes trying to access
	 * kernel addresses.
	 */
	l2 = pm->pm_l2[L2_IDX(l1idx)];
	if (l2 == NULL)
		goto out;

	/*
	 * Likewise if there is no L2 descriptor table
	 */
	l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];
	if (l2b->l2b_kva == NULL)
		goto out;

	/*
	 * Check the PTE itself.
	 */
	ptep = &l2b->l2b_kva[l2pte_index(va)];
	pte = *ptep;
	if (pte == 0)
		goto out;

	/*
	 * Catch a userland access to the vector page mapped at 0x0
	 */
	if (user && (pte & L2_S_PROT_U) == 0)
		goto out;
	if (va == vector_page)
		goto out;

	pa = l2pte_pa(pte);

	if ((ftype & VM_PROT_WRITE) && (pte & L2_S_PROT_W) == 0) {
		/*
		 * This looks like a good candidate for "page modified"
		 * emulation...
		 */
		struct pv_entry *pv;
		struct vm_page *pg;

		/* Extract the physical address of the page */
		if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL) {
			goto out;
		}
		/* Get the current flags for this page. */

		pv = pmap_find_pv(pg, pm, va);
		if (pv == NULL) {
			goto out;
		}

		/*
		 * Do the flags say this page is writable? If not then it
		 * is a genuine write fault. If yes then the write fault is
		 * our fault as we did not reflect the write access in the
		 * PTE. Now we know a write has occurred we can correct this
		 * and also set the modified bit
		 */
		if ((pv->pv_flags & PVF_WRITE) == 0) {
			goto out;
		}

		pg->md.pvh_attrs |= PVF_REF | PVF_MOD;
		vm_page_dirty(pg);
		pv->pv_flags |= PVF_REF | PVF_MOD;

		/* 
		 * Re-enable write permissions for the page.  No need to call
		 * pmap_vac_me_harder(), since this is just a
		 * modified-emulation fault, and the PVF_WRITE bit isn't
		 * changing. We've already set the cacheable bits based on
		 * the assumption that we can write to this page.
		 */
		*ptep = (pte & ~L2_TYPE_MASK) | L2_S_PROTO | L2_S_PROT_W;
		PTE_SYNC(ptep);
		rv = 1;
	} else
	if ((pte & L2_TYPE_MASK) == L2_TYPE_INV) {
		/*
		 * This looks like a good candidate for "page referenced"
		 * emulation.
		 */
		struct pv_entry *pv;
		struct vm_page *pg;

		/* Extract the physical address of the page */
		if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
			goto out;
		/* Get the current flags for this page. */

		pv = pmap_find_pv(pg, pm, va);
		if (pv == NULL)
			goto out;

		pg->md.pvh_attrs |= PVF_REF;
		pv->pv_flags |= PVF_REF;


		*ptep = (pte & ~L2_TYPE_MASK) | L2_S_PROTO;
		PTE_SYNC(ptep);
		rv = 1;
	}

	/*
	 * We know there is a valid mapping here, so simply
	 * fix up the L1 if necessary.
	 */
	pl1pd = &pm->pm_l1->l1_kva[l1idx];
	l1pd = l2b->l2b_phys | L1_C_DOM(pm->pm_domain) | L1_C_PROTO;
	if (*pl1pd != l1pd) {
		*pl1pd = l1pd;
		PTE_SYNC(pl1pd);
		rv = 1;
	}

#ifdef CPU_SA110
	/*
	 * There are bugs in the rev K SA110.  This is a check for one
	 * of them.
	 */
	if (rv == 0 && curcpu()->ci_arm_cputype == CPU_ID_SA110 &&
	    curcpu()->ci_arm_cpurev < 3) {
		/* Always current pmap */
		if (l2pte_valid(pte)) {
			extern int kernel_debug;
			if (kernel_debug & 1) {
				struct proc *p = curlwp->l_proc;
				printf("prefetch_abort: page is already "
				    "mapped - pte=%p *pte=%08x\n", ptep, pte);
				printf("prefetch_abort: pc=%08lx proc=%p "
				    "process=%s\n", va, p, p->p_comm);
				printf("prefetch_abort: far=%08x fs=%x\n",
				    cpu_faultaddress(), cpu_faultstatus());
			}
#ifdef DDB
			if (kernel_debug & 2)
				Debugger();
#endif
			rv = 1;
		}
	}
#endif /* CPU_SA110 */

#ifdef DEBUG
	/*
	 * If 'rv == 0' at this point, it generally indicates that there is a
	 * stale TLB entry for the faulting address. This happens when two or
	 * more processes are sharing an L1. Since we don't flush the TLB on
	 * a context switch between such processes, we can take domain faults
	 * for mappings which exist at the same VA in both processes. EVEN IF
	 * WE'VE RECENTLY FIXED UP THE CORRESPONDING L1 in pmap_enter(), for
	 * example.
	 *
	 * This is extremely likely to happen if pmap_enter() updated the L1
	 * entry for a recently entered mapping. In this case, the TLB is
	 * flushed for the new mapping, but there may still be TLB entries for
	 * other mappings belonging to other processes in the 1MB range
	 * covered by the L1 entry.
	 *
	 * Since 'rv == 0', we know that the L1 already contains the correct
	 * value, so the fault must be due to a stale TLB entry.
	 *
	 * Since we always need to flush the TLB anyway in the case where we
	 * fixed up the L1, or frobbed the L2 PTE, we effectively deal with
	 * stale TLB entries dynamically.
	 *
	 * However, the above condition can ONLY happen if the current L1 is
	 * being shared. If it happens when the L1 is unshared, it indicates
	 * that other parts of the pmap are not doing their job WRT managing
	 * the TLB.
	 */
	if (rv == 0 && pm->pm_l1->l1_domain_use_count == 1) {
		extern int last_fault_code;
		printf("fixup: pm %p, va 0x%lx, ftype %d - nothing to do!\n",
		    pm, va, ftype);
		printf("fixup: l2 %p, l2b %p, ptep %p, pl1pd %p\n",
		    l2, l2b, ptep, pl1pd);
		printf("fixup: pte 0x%x, l1pd 0x%x, last code 0x%x\n",
		    pte, l1pd, last_fault_code);
#ifdef DDB
		Debugger();
#endif
	}
#endif

	cpu_tlb_flushID_SE(va);
	cpu_cpwait();

	rv = 1;

out:
	vm_page_unlock_queues();
	PMAP_UNLOCK(pm);
	return (rv);
}

void
pmap_postinit(void)
{
	struct l2_bucket *l2b;
	struct l1_ttable *l1;
	pd_entry_t *pl1pt;
	pt_entry_t *ptep, pte;
	vm_offset_t va, eva;
	u_int loop, needed;
	
	needed = (maxproc / PMAP_DOMAINS) + ((maxproc % PMAP_DOMAINS) ? 1 : 0);
	needed -= 1;
	l1 = malloc(sizeof(*l1) * needed, M_VMPMAP, M_WAITOK);

	for (loop = 0; loop < needed; loop++, l1++) {
		/* Allocate a L1 page table */
		va = (vm_offset_t)contigmalloc(L1_TABLE_SIZE, M_VMPMAP, 0, 0x0,
		    0xffffffff, L1_TABLE_SIZE, 0);

		if (va == 0)
			panic("Cannot allocate L1 KVM");

		eva = va + L1_TABLE_SIZE;
		pl1pt = (pd_entry_t *)va;
		
		while (va < eva) {
				l2b = pmap_get_l2_bucket(pmap_kernel(), va);
				ptep = &l2b->l2b_kva[l2pte_index(va)];
				pte = *ptep;
				pte = (pte & ~L2_S_CACHE_MASK) | pte_l2_s_cache_mode_pt;
				*ptep = pte;
				PTE_SYNC(ptep);
				cpu_tlb_flushD_SE(va);
				
				va += PAGE_SIZE;
		}
		pmap_init_l1(l1, pl1pt);
	}


#ifdef DEBUG
	printf("pmap_postinit: Allocated %d static L1 descriptor tables\n",
	    needed);
#endif
}

/*
 * This is used to stuff certain critical values into the PCB where they
 * can be accessed quickly from cpu_switch() et al.
 */
void
pmap_set_pcb_pagedir(pmap_t pm, struct pcb *pcb)
{
	struct l2_bucket *l2b;

	pcb->pcb_pagedir = pm->pm_l1->l1_physaddr;
	pcb->pcb_dacr = (DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL * 2)) |
	    (DOMAIN_CLIENT << (pm->pm_domain * 2));

	if (vector_page < KERNBASE) {
		pcb->pcb_pl1vec = &pm->pm_l1->l1_kva[L1_IDX(vector_page)];
		l2b = pmap_get_l2_bucket(pm, vector_page);
		pcb->pcb_l1vec = l2b->l2b_phys | L1_C_PROTO |
	 	    L1_C_DOM(pm->pm_domain) | L1_C_DOM(PMAP_DOMAIN_KERNEL);
	} else
		pcb->pcb_pl1vec = NULL;
}

void
pmap_activate(struct thread *td)
{
	pmap_t pm;
	struct pcb *pcb;

	pm = vmspace_pmap(td->td_proc->p_vmspace);
	pcb = td->td_pcb;

	critical_enter();
	pmap_set_pcb_pagedir(pm, pcb);

	if (td == curthread) {
		u_int cur_dacr, cur_ttb;

		__asm __volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(cur_ttb));
		__asm __volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(cur_dacr));

		cur_ttb &= ~(L1_TABLE_SIZE - 1);

		if (cur_ttb == (u_int)pcb->pcb_pagedir &&
		    cur_dacr == pcb->pcb_dacr) {
			/*
			 * No need to switch address spaces.
			 */
			critical_exit();
			return;
		}


		/*
		 * We MUST, I repeat, MUST fix up the L1 entry corresponding
		 * to 'vector_page' in the incoming L1 table before switching
		 * to it otherwise subsequent interrupts/exceptions (including
		 * domain faults!) will jump into hyperspace.
		 */
		if (pcb->pcb_pl1vec) {

			*pcb->pcb_pl1vec = pcb->pcb_l1vec;
			/*
			 * Don't need to PTE_SYNC() at this point since
			 * cpu_setttb() is about to flush both the cache
			 * and the TLB.
			 */
		}

		cpu_domains(pcb->pcb_dacr);
		cpu_setttb(pcb->pcb_pagedir);
	}
	critical_exit();
}

static int
pmap_set_pt_cache_mode(pd_entry_t *kl1, vm_offset_t va)
{
	pd_entry_t *pdep, pde;
	pt_entry_t *ptep, pte;
	vm_offset_t pa;
	int rv = 0;

	/*
	 * Make sure the descriptor itself has the correct cache mode
	 */
	pdep = &kl1[L1_IDX(va)];
	pde = *pdep;

	if (l1pte_section_p(pde)) {
		if ((pde & L1_S_CACHE_MASK) != pte_l1_s_cache_mode_pt) {
			*pdep = (pde & ~L1_S_CACHE_MASK) |
			    pte_l1_s_cache_mode_pt;
			PTE_SYNC(pdep);
			cpu_dcache_wbinv_range((vm_offset_t)pdep,
			    sizeof(*pdep));
			rv = 1;
		}
	} else {
		pa = (vm_paddr_t)(pde & L1_C_ADDR_MASK);
		ptep = (pt_entry_t *)kernel_pt_lookup(pa);
		if (ptep == NULL)
			panic("pmap_bootstrap: No L2 for L2 @ va %p\n", ptep);

		ptep = &ptep[l2pte_index(va)];
		pte = *ptep;
		if ((pte & L2_S_CACHE_MASK) != pte_l2_s_cache_mode_pt) {
			*ptep = (pte & ~L2_S_CACHE_MASK) |
			    pte_l2_s_cache_mode_pt;
			PTE_SYNC(ptep);
			cpu_dcache_wbinv_range((vm_offset_t)ptep,
			    sizeof(*ptep));
			rv = 1;
		}
	}

	return (rv);
}

static void
pmap_alloc_specials(vm_offset_t *availp, int pages, vm_offset_t *vap, 
    pt_entry_t **ptep)
{
	vm_offset_t va = *availp;
	struct l2_bucket *l2b;

	if (ptep) {
		l2b = pmap_get_l2_bucket(pmap_kernel(), va);
		if (l2b == NULL)
			panic("pmap_alloc_specials: no l2b for 0x%x", va);

		*ptep = &l2b->l2b_kva[l2pte_index(va)];
	}

	*vap = va;
	*availp = va + (PAGE_SIZE * pages);
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *
 *	On the arm this is called after mapping has already been enabled
 *	and just syncs the pmap module with what has already been done.
 *	[We can't call it easily with mapping off since the kernel is not
 *	mapped with PA == VA, hence we would have to relocate every address
 *	from the linked base (virtual) address "KERNBASE" to the actual
 *	(physical) address starting relative to 0]
 */
#define PMAP_STATIC_L2_SIZE 16
#ifdef ARM_USE_SMALL_ALLOC
extern struct mtx smallalloc_mtx;
#endif

void
pmap_bootstrap(vm_offset_t firstaddr, vm_offset_t lastaddr, struct pv_addr *l1pt)
{
	static struct l1_ttable static_l1;
	static struct l2_dtable static_l2[PMAP_STATIC_L2_SIZE];
	struct l1_ttable *l1 = &static_l1;
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	pd_entry_t pde;
	pd_entry_t *kernel_l1pt = (pd_entry_t *)l1pt->pv_va;
	pt_entry_t *ptep;
	vm_paddr_t pa;
	vm_offset_t va;
	vm_size_t size;
	int l1idx, l2idx, l2next = 0;

	PDEBUG(1, printf("firstaddr = %08x, loadaddr = %08x\n",
	    firstaddr, loadaddr));
	
	virtual_avail = firstaddr;
	kernel_pmap = &kernel_pmap_store;
	kernel_pmap->pm_l1 = l1;
	kernel_l1pa = l1pt->pv_pa;
	
	/*
	 * Scan the L1 translation table created by initarm() and create
	 * the required metadata for all valid mappings found in it.
	 */
	for (l1idx = 0; l1idx < (L1_TABLE_SIZE / sizeof(pd_entry_t)); l1idx++) {
		pde = kernel_l1pt[l1idx];

		/*
		 * We're only interested in Coarse mappings.
		 * pmap_extract() can deal with section mappings without
		 * recourse to checking L2 metadata.
		 */
		if ((pde & L1_TYPE_MASK) != L1_TYPE_C)
			continue;

		/*
		 * Lookup the KVA of this L2 descriptor table
		 */
		pa = (vm_paddr_t)(pde & L1_C_ADDR_MASK);
		ptep = (pt_entry_t *)kernel_pt_lookup(pa);
		
		if (ptep == NULL) {
			panic("pmap_bootstrap: No L2 for va 0x%x, pa 0x%lx",
			    (u_int)l1idx << L1_S_SHIFT, (long unsigned int)pa);
		}

		/*
		 * Fetch the associated L2 metadata structure.
		 * Allocate a new one if necessary.
		 */
		if ((l2 = kernel_pmap->pm_l2[L2_IDX(l1idx)]) == NULL) {
			if (l2next == PMAP_STATIC_L2_SIZE)
				panic("pmap_bootstrap: out of static L2s");
			kernel_pmap->pm_l2[L2_IDX(l1idx)] = l2 = 
			    &static_l2[l2next++];
		}

		/*
		 * One more L1 slot tracked...
		 */
		l2->l2_occupancy++;

		/*
		 * Fill in the details of the L2 descriptor in the
		 * appropriate bucket.
		 */
		l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];
		l2b->l2b_kva = ptep;
		l2b->l2b_phys = pa;
		l2b->l2b_l1idx = l1idx;

		/*
		 * Establish an initial occupancy count for this descriptor
		 */
		for (l2idx = 0;
		    l2idx < (L2_TABLE_SIZE_REAL / sizeof(pt_entry_t));
		    l2idx++) {
			if ((ptep[l2idx] & L2_TYPE_MASK) != L2_TYPE_INV) {
				l2b->l2b_occupancy++;
			}
		}

		/*
		 * Make sure the descriptor itself has the correct cache mode.
		 * If not, fix it, but whine about the problem. Port-meisters
		 * should consider this a clue to fix up their initarm()
		 * function. :)
		 */
		if (pmap_set_pt_cache_mode(kernel_l1pt, (vm_offset_t)ptep)) {
			printf("pmap_bootstrap: WARNING! wrong cache mode for "
			    "L2 pte @ %p\n", ptep);
		}
	}

	
	/*
	 * Ensure the primary (kernel) L1 has the correct cache mode for
	 * a page table. Bitch if it is not correctly set.
	 */
	for (va = (vm_offset_t)kernel_l1pt;
	    va < ((vm_offset_t)kernel_l1pt + L1_TABLE_SIZE); va += PAGE_SIZE) {
		if (pmap_set_pt_cache_mode(kernel_l1pt, va))
			printf("pmap_bootstrap: WARNING! wrong cache mode for "
			    "primary L1 @ 0x%x\n", va);
	}

	cpu_dcache_wbinv_all();
	cpu_tlb_flushID();
	cpu_cpwait();

	PMAP_LOCK_INIT(kernel_pmap);
	kernel_pmap->pm_active = -1;
	kernel_pmap->pm_domain = PMAP_DOMAIN_KERNEL;
	TAILQ_INIT(&kernel_pmap->pm_pvlist);
	
	/*
	 * Reserve some special page table entries/VA space for temporary
	 * mapping of pages.
	 */
#define SYSMAP(c, p, v, n)						\
    v = (c)va; va += ((n)*PAGE_SIZE); p = pte; pte += (n);
    
	pmap_alloc_specials(&virtual_avail, 1, &csrcp, &csrc_pte);
	pmap_set_pt_cache_mode(kernel_l1pt, (vm_offset_t)csrc_pte);
	pmap_alloc_specials(&virtual_avail, 1, &cdstp, &cdst_pte);
	pmap_set_pt_cache_mode(kernel_l1pt, (vm_offset_t)cdst_pte);
	size = ((lastaddr - pmap_curmaxkvaddr) + L1_S_OFFSET) / L1_S_SIZE;
	pmap_alloc_specials(&virtual_avail,
	    round_page(size * L2_TABLE_SIZE_REAL) / PAGE_SIZE,
	    &pmap_kernel_l2ptp_kva, NULL);
	
	size = (size + (L2_BUCKET_SIZE - 1)) / L2_BUCKET_SIZE;
	pmap_alloc_specials(&virtual_avail,
	    round_page(size * sizeof(struct l2_dtable)) / PAGE_SIZE,
	    &pmap_kernel_l2dtable_kva, NULL);

	pmap_alloc_specials(&virtual_avail,
	    1, (vm_offset_t*)&_tmppt, NULL);
	SLIST_INIT(&l1_list);
	TAILQ_INIT(&l1_lru_list);
	mtx_init(&l1_lru_lock, "l1 list lock", NULL, MTX_DEF);
	pmap_init_l1(l1, kernel_l1pt);
	cpu_dcache_wbinv_all();

	virtual_avail = round_page(virtual_avail);
	virtual_end = lastaddr;
	kernel_vm_end = pmap_curmaxkvaddr;
	arm_nocache_startaddr = lastaddr;
	mtx_init(&cmtx, "TMP mappings mtx", NULL, MTX_DEF);

#ifdef ARM_USE_SMALL_ALLOC
	mtx_init(&smallalloc_mtx, "Small alloc page list", NULL, MTX_DEF);
	arm_init_smallalloc();
#endif
	pmap_set_pcb_pagedir(kernel_pmap, thread0.td_pcb);
}

/***************************************************
 * Pmap allocation/deallocation routines.
 ***************************************************/

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap_t pmap)
{
	struct pcb *pcb;
	
	pmap_idcache_wbinv_all(pmap);
	pmap_tlb_flushID(pmap);
	cpu_cpwait();
	if (vector_page < KERNBASE) {
		struct pcb *curpcb = PCPU_GET(curpcb);
		pcb = thread0.td_pcb;
		if (pmap_is_current(pmap)) {
			/*
 			 * Frob the L1 entry corresponding to the vector
			 * page so that it contains the kernel pmap's domain
			 * number. This will ensure pmap_remove() does not
			 * pull the current vector page out from under us.
			 */
			critical_enter();
			*pcb->pcb_pl1vec = pcb->pcb_l1vec;
			cpu_domains(pcb->pcb_dacr);
			cpu_setttb(pcb->pcb_pagedir);
			critical_exit();
		}
		pmap_remove(pmap, vector_page, vector_page + PAGE_SIZE);
		/*
		 * Make sure cpu_switch(), et al, DTRT. This is safe to do
		 * since this process has no remaining mappings of its own.
		 */
		curpcb->pcb_pl1vec = pcb->pcb_pl1vec;
		curpcb->pcb_l1vec = pcb->pcb_l1vec;
		curpcb->pcb_dacr = pcb->pcb_dacr;
		curpcb->pcb_pagedir = pcb->pcb_pagedir;

	}
	pmap_free_l1(pmap);
	PMAP_LOCK_DESTROY(pmap);
	
	dprintf("pmap_release()\n");
}



/*
 * Helper function for pmap_grow_l2_bucket()
 */
static __inline int
pmap_grow_map(vm_offset_t va, pt_entry_t cache_mode, vm_paddr_t *pap)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep;
	vm_paddr_t pa;
	struct vm_page *pg;
	
	pg = vm_page_alloc(NULL, 0, VM_ALLOC_NOOBJ | VM_ALLOC_WIRED);
	if (pg == NULL)
		return (1);
	pa = VM_PAGE_TO_PHYS(pg);

	if (pap)
		*pap = pa;

	l2b = pmap_get_l2_bucket(pmap_kernel(), va);

	ptep = &l2b->l2b_kva[l2pte_index(va)];
	*ptep = L2_S_PROTO | pa | cache_mode |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ | VM_PROT_WRITE);
	PTE_SYNC(ptep);
	return (0);
}

/*
 * This is the same as pmap_alloc_l2_bucket(), except that it is only
 * used by pmap_growkernel().
 */
static __inline struct l2_bucket *
pmap_grow_l2_bucket(pmap_t pm, vm_offset_t va)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	struct l1_ttable *l1;
	pd_entry_t *pl1pd;
	u_short l1idx;
	vm_offset_t nva;

	l1idx = L1_IDX(va);

	if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL) {
		/*
		 * No mapping at this address, as there is
		 * no entry in the L1 table.
		 * Need to allocate a new l2_dtable.
		 */
		nva = pmap_kernel_l2dtable_kva;
		if ((nva & PAGE_MASK) == 0) {
			/*
			 * Need to allocate a backing page
			 */
			if (pmap_grow_map(nva, pte_l2_s_cache_mode, NULL))
				return (NULL);
		}

		l2 = (struct l2_dtable *)nva;
		nva += sizeof(struct l2_dtable);

		if ((nva & PAGE_MASK) < (pmap_kernel_l2dtable_kva & 
		    PAGE_MASK)) {
			/*
			 * The new l2_dtable straddles a page boundary.
			 * Map in another page to cover it.
			 */
			if (pmap_grow_map(nva, pte_l2_s_cache_mode, NULL))
				return (NULL);
		}

		pmap_kernel_l2dtable_kva = nva;

		/*
		 * Link it into the parent pmap
		 */
		pm->pm_l2[L2_IDX(l1idx)] = l2;
		memset(l2, 0, sizeof(*l2));
	}

	l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];

	/*
	 * Fetch pointer to the L2 page table associated with the address.
	 */
	if (l2b->l2b_kva == NULL) {
		pt_entry_t *ptep;

		/*
		 * No L2 page table has been allocated. Chances are, this
		 * is because we just allocated the l2_dtable, above.
		 */
		nva = pmap_kernel_l2ptp_kva;
		ptep = (pt_entry_t *)nva;
		if ((nva & PAGE_MASK) == 0) {
			/*
			 * Need to allocate a backing page
			 */
			if (pmap_grow_map(nva, pte_l2_s_cache_mode_pt,
			    &pmap_kernel_l2ptp_phys))
				return (NULL);
			PTE_SYNC_RANGE(ptep, PAGE_SIZE / sizeof(pt_entry_t));
		}
		memset(ptep, 0, L2_TABLE_SIZE_REAL);
		l2->l2_occupancy++;
		l2b->l2b_kva = ptep;
		l2b->l2b_l1idx = l1idx;
		l2b->l2b_phys = pmap_kernel_l2ptp_phys;

		pmap_kernel_l2ptp_kva += L2_TABLE_SIZE_REAL;
		pmap_kernel_l2ptp_phys += L2_TABLE_SIZE_REAL;
	}

	/* Distribute new L1 entry to all other L1s */
	SLIST_FOREACH(l1, &l1_list, l1_link) {
			pl1pd = &l1->l1_kva[L1_IDX(va)];
			*pl1pd = l2b->l2b_phys | L1_C_DOM(PMAP_DOMAIN_KERNEL) |
			    L1_C_PROTO;
			PTE_SYNC(pl1pd);
	}

	return (l2b);
}


/*
 * grow the number of kernel page table entries, if needed
 */
void
pmap_growkernel(vm_offset_t addr)
{
	pmap_t kpm = pmap_kernel();

	if (addr <= pmap_curmaxkvaddr)
		return;		/* we are OK */

	/*
	 * whoops!   we need to add kernel PTPs
	 */

	/* Map 1MB at a time */
	for (; pmap_curmaxkvaddr < addr; pmap_curmaxkvaddr += L1_S_SIZE)
		pmap_grow_l2_bucket(kpm, pmap_curmaxkvaddr);

	/*
	 * flush out the cache, expensive but growkernel will happen so
	 * rarely
	 */
	cpu_dcache_wbinv_all();
	cpu_tlb_flushD();
	cpu_cpwait();
	kernel_vm_end = pmap_curmaxkvaddr;

}


/*
 * Remove all pages from specified address space
 * this aids process exit speeds.  Also, this code
 * is special cased for current process only, but
 * can have the more generic (and slightly slower)
 * mode enabled.  This is much faster than pmap_remove
 * in the case of running down an entire address space.
 */
void
pmap_remove_pages(pmap_t pmap)
{
	struct pv_entry *pv, *npv;
	struct l2_bucket *l2b = NULL;
	vm_page_t m;
	pt_entry_t *pt;
	
	vm_page_lock_queues();
	PMAP_LOCK(pmap);
	for (pv = TAILQ_FIRST(&pmap->pm_pvlist); pv; pv = npv) {
		if (pv->pv_flags & PVF_WIRED) {
			/* The page is wired, cannot remove it now. */
			npv = TAILQ_NEXT(pv, pv_plist);
			continue;
		}
		pmap->pm_stats.resident_count--;
		l2b = pmap_get_l2_bucket(pmap, pv->pv_va);
		KASSERT(l2b != NULL, ("No L2 bucket in pmap_remove_pages"));
		pt = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
		m = PHYS_TO_VM_PAGE(*pt & L2_ADDR_MASK);
#ifdef ARM_USE_SMALL_ALLOC
		KASSERT((vm_offset_t)m >= alloc_firstaddr, ("Trying to access non-existent page va %x pte %x", pv->pv_va, *pt));
#else
		KASSERT((vm_offset_t)m >= KERNBASE, ("Trying to access non-existent page va %x pte %x", pv->pv_va, *pt));
#endif
		*pt = 0;
		PTE_SYNC(pt);
		npv = TAILQ_NEXT(pv, pv_plist);
		pmap_nuke_pv(m, pmap, pv);
		if (TAILQ_EMPTY(&m->md.pv_list))
			vm_page_flag_clear(m, PG_WRITEABLE);
		pmap_free_pv_entry(pv);
		pmap_free_l2_bucket(pmap, l2b, 1);
	}
	vm_page_unlock_queues();
	cpu_idcache_wbinv_all();
	cpu_tlb_flushID();
	cpu_cpwait();
	PMAP_UNLOCK(pmap);
}


/***************************************************
 * Low level mapping routines.....
 ***************************************************/

#ifdef ARM_HAVE_SUPERSECTIONS
/* Map a super section into the KVA. */

void
pmap_kenter_supersection(vm_offset_t va, uint64_t pa, int flags)
{
	pd_entry_t pd = L1_S_PROTO | L1_S_SUPERSEC | (pa & L1_SUP_FRAME) |
	    (((pa >> 32) & 0xf) << 20) | L1_S_PROT(PTE_KERNEL,
	    VM_PROT_READ|VM_PROT_WRITE) | L1_S_DOM(PMAP_DOMAIN_KERNEL);
	struct l1_ttable *l1;	
	vm_offset_t va0, va_end;

	KASSERT(((va | pa) & L1_SUP_OFFSET) == 0,
	    ("Not a valid super section mapping"));
	if (flags & SECTION_CACHE)
		pd |= pte_l1_s_cache_mode;
	else if (flags & SECTION_PT)
		pd |= pte_l1_s_cache_mode_pt;
	va0 = va & L1_SUP_FRAME;
	va_end = va + L1_SUP_SIZE;
	SLIST_FOREACH(l1, &l1_list, l1_link) {
		va = va0;
		for (; va < va_end; va += L1_S_SIZE) {
			l1->l1_kva[L1_IDX(va)] = pd;
			PTE_SYNC(&l1->l1_kva[L1_IDX(va)]);
		}
	}
}
#endif

/* Map a section into the KVA. */

void
pmap_kenter_section(vm_offset_t va, vm_offset_t pa, int flags)
{
	pd_entry_t pd = L1_S_PROTO | pa | L1_S_PROT(PTE_KERNEL,
	    VM_PROT_READ|VM_PROT_WRITE) | L1_S_DOM(PMAP_DOMAIN_KERNEL);
	struct l1_ttable *l1;

	KASSERT(((va | pa) & L1_S_OFFSET) == 0,
	    ("Not a valid section mapping"));
	if (flags & SECTION_CACHE)
		pd |= pte_l1_s_cache_mode;
	else if (flags & SECTION_PT)
		pd |= pte_l1_s_cache_mode_pt;
	SLIST_FOREACH(l1, &l1_list, l1_link) {
		l1->l1_kva[L1_IDX(va)] = pd;
		PTE_SYNC(&l1->l1_kva[L1_IDX(va)]);
	}
}

/*
 * add a wired page to the kva
 * note that in order for the mapping to take effect -- you
 * should do a invltlb after doing the pmap_kenter...
 */
static PMAP_INLINE void
pmap_kenter_internal(vm_offset_t va, vm_offset_t pa, int flags)
{
	struct l2_bucket *l2b;
	pt_entry_t *pte;
	pt_entry_t opte;
	PDEBUG(1, printf("pmap_kenter: va = %08x, pa = %08x\n",
	    (uint32_t) va, (uint32_t) pa));


	l2b = pmap_get_l2_bucket(pmap_kernel(), va);
	if (l2b == NULL)
		l2b = pmap_grow_l2_bucket(pmap_kernel(), va);
	KASSERT(l2b != NULL, ("No L2 Bucket"));
	pte = &l2b->l2b_kva[l2pte_index(va)];
	opte = *pte;
	PDEBUG(1, printf("pmap_kenter: pte = %08x, opte = %08x, npte = %08x\n",
	    (uint32_t) pte, opte, *pte));
	if (l2pte_valid(opte)) {
		cpu_dcache_wbinv_range(va, PAGE_SIZE);
		cpu_tlb_flushD_SE(va);
		cpu_cpwait();
	} else {
		if (opte == 0)
			l2b->l2b_occupancy++;
	}
	*pte = L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, 
	    VM_PROT_READ | VM_PROT_WRITE);
	if (flags & KENTER_CACHE)
		*pte |= pte_l2_s_cache_mode;
	if (flags & KENTER_USER)
		*pte |= L2_S_PROT_U;
	PTE_SYNC(pte);
}

void
pmap_kenter(vm_offset_t va, vm_paddr_t pa)
{
	pmap_kenter_internal(va, pa, KENTER_CACHE);
}

void
pmap_kenter_nocache(vm_offset_t va, vm_paddr_t pa)
{

	pmap_kenter_internal(va, pa, 0);
}

void
pmap_kenter_user(vm_offset_t va, vm_paddr_t pa)
{

	pmap_kenter_internal(va, pa, KENTER_CACHE|KENTER_USER);
	/*
	 * Call pmap_fault_fixup now, to make sure we'll have no exception
	 * at the first use of the new address, or bad things will happen,
	 * as we use one of these addresses in the exception handlers.
	 */
	pmap_fault_fixup(pmap_kernel(), va, VM_PROT_READ|VM_PROT_WRITE, 1);
}

/*
 * remove a page rom the kernel pagetables
 */
void
pmap_kremove(vm_offset_t va)
{
	struct l2_bucket *l2b;
	pt_entry_t *pte, opte;
		
	l2b = pmap_get_l2_bucket(pmap_kernel(), va);
	if (!l2b)
		return;
	KASSERT(l2b != NULL, ("No L2 Bucket"));
	pte = &l2b->l2b_kva[l2pte_index(va)];
	opte = *pte;
	if (l2pte_valid(opte)) {
		cpu_dcache_wbinv_range(va, PAGE_SIZE);
		cpu_tlb_flushD_SE(va);
		cpu_cpwait();
		*pte = 0;
	}
}


/*
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	The value passed in '*virt' is a suggested virtual address for
 *	the mapping. Architectures which can support a direct-mapped
 *	physical to virtual region can return the appropriate address
 *	within that region, leaving '*virt' unchanged. Other
 *	architectures should map the pages starting at '*virt' and
 *	update '*virt' with the first usable address after the mapped
 *	region.
 */
vm_offset_t
pmap_map(vm_offset_t *virt, vm_offset_t start, vm_offset_t end, int prot)
{
#ifdef ARM_USE_SMALL_ALLOC
	return (arm_ptovirt(start));
#else
	vm_offset_t sva = *virt;
	vm_offset_t va = sva;

	PDEBUG(1, printf("pmap_map: virt = %08x, start = %08x, end = %08x, "
	    "prot = %d\n", (uint32_t) *virt, (uint32_t) start, (uint32_t) end,
	    prot));
	    
	while (start < end) {
		pmap_kenter(va, start);
		va += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	*virt = va;
	return (sva);
#endif
}

static void
pmap_wb_page(vm_page_t m)
{
	struct pv_entry *pv;

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_list)
	    pmap_dcache_wb_range(pv->pv_pmap, pv->pv_va, PAGE_SIZE, FALSE,
		(pv->pv_flags & PVF_WRITE) == 0);
}

static void
pmap_inv_page(vm_page_t m)
{
	struct pv_entry *pv;

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_list)
	    pmap_dcache_wb_range(pv->pv_pmap, pv->pv_va, PAGE_SIZE, TRUE, TRUE);
}
/*
 * Add a list of wired pages to the kva
 * this routine is only used for temporary
 * kernel mappings that do not need to have
 * page modification or references recorded.
 * Note that old mappings are simply written
 * over.  The page *must* be wired.
 */
void
pmap_qenter(vm_offset_t va, vm_page_t *m, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		pmap_wb_page(m[i]);
		pmap_kenter_internal(va, VM_PAGE_TO_PHYS(m[i]), 
		    KENTER_CACHE);
		va += PAGE_SIZE;
	}
}


/*
 * this routine jerks page mappings from the
 * kernel -- it is meant only for temporary mappings.
 */
void
pmap_qremove(vm_offset_t va, int count)
{
	vm_paddr_t pa;
	int i;

	for (i = 0; i < count; i++) {
		pa = vtophys(va);
		if (pa) {
			pmap_inv_page(PHYS_TO_VM_PAGE(pa));
			pmap_kremove(va);
		}
		va += PAGE_SIZE;
	}
}


/*
 * pmap_object_init_pt preloads the ptes for a given object
 * into the specified pmap.  This eliminates the blast of soft
 * faults on process startup and immediately after an mmap.
 */
void
pmap_object_init_pt(pmap_t pmap, vm_offset_t addr, vm_object_t object,
    vm_pindex_t pindex, vm_size_t size)
{

	VM_OBJECT_LOCK_ASSERT(object, MA_OWNED);
	KASSERT(object->type == OBJT_DEVICE,
	    ("pmap_object_init_pt: non-device object"));
}


/*
 *	pmap_is_prefaultable:
 *
 *	Return whether or not the specified virtual address is elgible
 *	for prefault.
 */
boolean_t
pmap_is_prefaultable(pmap_t pmap, vm_offset_t addr)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	if (!pmap_get_pde_pte(pmap, addr, &pde, &pte))
		return (FALSE);
	KASSERT(pte != NULL, ("Valid mapping but no pte ?"));
	if (*pte == 0)
		return (TRUE);
	return (FALSE);
}

/*
 * Fetch pointers to the PDE/PTE for the given pmap/VA pair.
 * Returns TRUE if the mapping exists, else FALSE.
 *
 * NOTE: This function is only used by a couple of arm-specific modules.
 * It is not safe to take any pmap locks here, since we could be right
 * in the middle of debugging the pmap anyway...
 *
 * It is possible for this routine to return FALSE even though a valid
 * mapping does exist. This is because we don't lock, so the metadata
 * state may be inconsistent.
 *
 * NOTE: We can return a NULL *ptp in the case where the L1 pde is
 * a "section" mapping.
 */
boolean_t
pmap_get_pde_pte(pmap_t pm, vm_offset_t va, pd_entry_t **pdp, pt_entry_t **ptp)
{
	struct l2_dtable *l2;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep;
	u_short l1idx;

	if (pm->pm_l1 == NULL)
		return (FALSE);

	l1idx = L1_IDX(va);
	*pdp = pl1pd = &pm->pm_l1->l1_kva[l1idx];
	l1pd = *pl1pd;

	if (l1pte_section_p(l1pd)) {
		*ptp = NULL;
		return (TRUE);
	}

	if (pm->pm_l2 == NULL)
		return (FALSE);

	l2 = pm->pm_l2[L2_IDX(l1idx)];

	if (l2 == NULL ||
	    (ptep = l2->l2_bucket[L2_BUCKET(l1idx)].l2b_kva) == NULL) {
		return (FALSE);
	}

	*ptp = &ptep[l2pte_index(va)];
	return (TRUE);
}

/*
 *      Routine:        pmap_remove_all
 *      Function:
 *              Removes this physical page from
 *              all physical maps in which it resides.
 *              Reflects back modify bits to the pager.
 *
 *      Notes:
 *              Original versions of this routine were very
 *              inefficient because they iteratively called
 *              pmap_remove (slow...)
 */
void
pmap_remove_all(vm_page_t m)
{
	pv_entry_t pv;
	pt_entry_t *ptep, pte;
	struct l2_bucket *l2b;
	boolean_t flush = FALSE;
	pmap_t curpm;
	int flags = 0;

#if defined(PMAP_DEBUG)
	/*
	 * XXX This makes pmap_remove_all() illegal for non-managed pages!
	 */
	if (m->flags & PG_FICTITIOUS) {
		panic("pmap_remove_all: illegal for unmanaged page, va: 0x%x", VM_PAGE_TO_PHYS(m));
	}
#endif

	if (TAILQ_EMPTY(&m->md.pv_list))
		return;
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	curpm = vmspace_pmap(curproc->p_vmspace);
	while ((pv = TAILQ_FIRST(&m->md.pv_list)) != NULL) {
		if (flush == FALSE && (pv->pv_pmap == curpm ||
		    pv->pv_pmap == pmap_kernel()))
			flush = TRUE;
		PMAP_LOCK(pv->pv_pmap);
		l2b = pmap_get_l2_bucket(pv->pv_pmap, pv->pv_va);
		KASSERT(l2b != NULL, ("No l2 bucket"));
		ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
		pte = *ptep;
		*ptep = 0;
		PTE_SYNC_CURRENT(pv->pv_pmap, ptep);
		pmap_free_l2_bucket(pv->pv_pmap, l2b, 1);
		if (pv->pv_flags & PVF_WIRED)
			pv->pv_pmap->pm_stats.wired_count--;
		pv->pv_pmap->pm_stats.resident_count--;
		flags |= pv->pv_flags;
		pmap_nuke_pv(m, pv->pv_pmap, pv);
		PMAP_UNLOCK(pv->pv_pmap);
		pmap_free_pv_entry(pv);
	}

	if (flush) {
		if (PV_BEEN_EXECD(flags))
			pmap_tlb_flushID(curpm);
		else
			pmap_tlb_flushD(curpm);
	}
	vm_page_flag_clear(m, PG_WRITEABLE);
}


/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pm, vm_offset_t sva, vm_offset_t eva, vm_prot_t prot)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	vm_offset_t next_bucket;
	u_int flags;
	int flush;

	if ((prot & VM_PROT_READ) == 0) {
		pmap_remove(pm, sva, eva);
		return;
	}

	if (prot & VM_PROT_WRITE) {
		/*
		 * If this is a read->write transition, just ignore it and let
		 * vm_fault() take care of it later.
		 */
		return;
	}

	vm_page_lock_queues();
	PMAP_LOCK(pm);

	/*
	 * OK, at this point, we know we're doing write-protect operation.
	 * If the pmap is active, write-back the range.
	 */
	pmap_dcache_wb_range(pm, sva, eva - sva, FALSE, FALSE);

	flush = ((eva - sva) >= (PAGE_SIZE * 4)) ? 0 : -1;
	flags = 0;

	while (sva < eva) {
		next_bucket = L2_NEXT_BUCKET(sva);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pm, sva);
		if (l2b == NULL) {
			sva = next_bucket;
			continue;
		}

		ptep = &l2b->l2b_kva[l2pte_index(sva)];

		while (sva < next_bucket) {
			if ((pte = *ptep) != 0 && (pte & L2_S_PROT_W) != 0) {
				struct vm_page *pg;
				u_int f;

				pg = PHYS_TO_VM_PAGE(l2pte_pa(pte));
				pte &= ~L2_S_PROT_W;
				*ptep = pte;
				PTE_SYNC(ptep);

				if (pg != NULL) {
					f = pmap_modify_pv(pg, pm, sva,
					    PVF_WRITE, 0);
					vm_page_dirty(pg);
				} else
					f = PVF_REF | PVF_EXEC;

				if (flush >= 0) {
					flush++;
					flags |= f;
				} else
				if (PV_BEEN_EXECD(f))
					pmap_tlb_flushID_SE(pm, sva);
				else
				if (PV_BEEN_REFD(f))
					pmap_tlb_flushD_SE(pm, sva);
			}

			sva += PAGE_SIZE;
			ptep++;
		}
	}


	if (flush) {
		if (PV_BEEN_EXECD(flags))
			pmap_tlb_flushID(pm);
		else
		if (PV_BEEN_REFD(flags))
			pmap_tlb_flushD(pm);
	}
	vm_page_unlock_queues();

 	PMAP_UNLOCK(pm);
}


/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */

void
pmap_enter(pmap_t pmap, vm_offset_t va, vm_prot_t access, vm_page_t m,
    vm_prot_t prot, boolean_t wired)
{

	vm_page_lock_queues();
	PMAP_LOCK(pmap);
	pmap_enter_locked(pmap, va, m, prot, wired, M_WAITOK);
	vm_page_unlock_queues();
 	PMAP_UNLOCK(pmap);
}

/*
 *	The page queues and pmap must be locked.
 */
static void
pmap_enter_locked(pmap_t pmap, vm_offset_t va, vm_page_t m, vm_prot_t prot,
    boolean_t wired, int flags)
{
	struct l2_bucket *l2b = NULL;
	struct vm_page *opg;
	struct pv_entry *pve = NULL;
	pt_entry_t *ptep, npte, opte;
	u_int nflags;
	u_int oflags;
	vm_paddr_t pa;

	PMAP_ASSERT_LOCKED(pmap);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if (va == vector_page) {
		pa = systempage.pv_pa;
		m = NULL;
	} else
		pa = VM_PAGE_TO_PHYS(m);
	nflags = 0;
	if (prot & VM_PROT_WRITE)
		nflags |= PVF_WRITE;
	if (prot & VM_PROT_EXECUTE)
		nflags |= PVF_EXEC;
	if (wired)
		nflags |= PVF_WIRED;
	PDEBUG(1, printf("pmap_enter: pmap = %08x, va = %08x, m = %08x, prot = %x, "
	    "wired = %x\n", (uint32_t) pmap, va, (uint32_t) m, prot, wired));
	    
	if (pmap == pmap_kernel()) {
		l2b = pmap_get_l2_bucket(pmap, va);
		if (l2b == NULL)
			l2b = pmap_grow_l2_bucket(pmap, va);
	} else {
do_l2b_alloc:
		l2b = pmap_alloc_l2_bucket(pmap, va);
		if (l2b == NULL) {
			if (flags & M_WAITOK) {
				PMAP_UNLOCK(pmap);
				vm_page_unlock_queues();
				VM_WAIT;
				vm_page_lock_queues();
				PMAP_LOCK(pmap);
				goto do_l2b_alloc;
			}
			return;
		}
	}

	ptep = &l2b->l2b_kva[l2pte_index(va)];
		    
	opte = *ptep;
	npte = pa;
	oflags = 0;
	if (opte) {
		/*
		 * There is already a mapping at this address.
		 * If the physical address is different, lookup the
		 * vm_page.
		 */
		if (l2pte_pa(opte) != pa)
			opg = PHYS_TO_VM_PAGE(l2pte_pa(opte));
		else
			opg = m;
	} else
		opg = NULL;

	if ((prot & (VM_PROT_ALL)) ||
	    (!m || m->md.pvh_attrs & PVF_REF)) {
		/*
		 * - The access type indicates that we don't need
		 *   to do referenced emulation.
		 * OR
		 * - The physical page has already been referenced
		 *   so no need to re-do referenced emulation here.
		 */
		npte |= L2_S_PROTO;
		
		nflags |= PVF_REF;
		
		if (m && ((prot & VM_PROT_WRITE) != 0 ||
		    (m->md.pvh_attrs & PVF_MOD))) {
			/*
			 * This is a writable mapping, and the
			 * page's mod state indicates it has
			 * already been modified. Make it
			 * writable from the outset.
			 */
			nflags |= PVF_MOD;
			if (!(m->md.pvh_attrs & PVF_MOD))
				vm_page_dirty(m);
		}
		if (m && opte)
			vm_page_flag_set(m, PG_REFERENCED);
	} else {
		/*
		 * Need to do page referenced emulation.
		 */
		npte |= L2_TYPE_INV;
	}
	
	if (prot & VM_PROT_WRITE) {
		npte |= L2_S_PROT_W;
		if (m != NULL)
			vm_page_flag_set(m, PG_WRITEABLE);
	}
	npte |= pte_l2_s_cache_mode;
	if (m && m == opg) {
		/*
		 * We're changing the attrs of an existing mapping.
		 */
		oflags = pmap_modify_pv(m, pmap, va,
		    PVF_WRITE | PVF_EXEC | PVF_WIRED |
		    PVF_MOD | PVF_REF, nflags);
		
		/*
		 * We may need to flush the cache if we're
		 * doing rw-ro...
		 */
		if (pmap_is_current(pmap) &&
		    (oflags & PVF_NC) == 0 &&
			    (opte & L2_S_PROT_W) != 0 &&
			    (prot & VM_PROT_WRITE) == 0)
			cpu_dcache_wb_range(va, PAGE_SIZE);
	} else {
		/*
		 * New mapping, or changing the backing page
		 * of an existing mapping.
		 */
		if (opg) {
			/*
			 * Replacing an existing mapping with a new one.
			 * It is part of our managed memory so we
			 * must remove it from the PV list
			 */
			pve = pmap_remove_pv(opg, pmap, va);
			if (m && (m->flags & (PG_UNMANAGED | PG_FICTITIOUS)) &&
			    pve)
				pmap_free_pv_entry(pve);
			else if (!pve && 
			    !(m->flags & (PG_UNMANAGED | PG_FICTITIOUS)))
				pve = pmap_get_pv_entry();
			KASSERT(pve != NULL || m->flags & (PG_UNMANAGED | 
			    PG_FICTITIOUS), ("No pv"));
			oflags = pve->pv_flags;
			
			/*
			 * If the old mapping was valid (ref/mod
			 * emulation creates 'invalid' mappings
			 * initially) then make sure to frob
			 * the cache.
			 */
			if ((oflags & PVF_NC) == 0 &&
			    l2pte_valid(opte)) {
				if (PV_BEEN_EXECD(oflags)) {
					pmap_idcache_wbinv_range(pmap, va,
					    PAGE_SIZE);
				} else
					if (PV_BEEN_REFD(oflags)) {
						pmap_dcache_wb_range(pmap, va,
						    PAGE_SIZE, TRUE,
						    (oflags & PVF_WRITE) == 0);
					}
			}
		} else if (m && !(m->flags & (PG_UNMANAGED | PG_FICTITIOUS)))
			if ((pve = pmap_get_pv_entry()) == NULL) {
				panic("pmap_enter: no pv entries");	
			}
		if (m && !(m->flags & (PG_UNMANAGED | PG_FICTITIOUS))) {
			KASSERT(va < kmi.clean_sva || va >= kmi.clean_eva,
			    ("pmap_enter: managed mapping within the clean submap"));
			pmap_enter_pv(m, pve, pmap, va, nflags);
		}
	}
	/*
	 * Make sure userland mappings get the right permissions
	 */
	if (pmap != pmap_kernel() && va != vector_page) {
		npte |= L2_S_PROT_U;
	}

	/*
	 * Keep the stats up to date
	 */
	if (opte == 0) {
		l2b->l2b_occupancy++;
		pmap->pm_stats.resident_count++;
	} 


	/*
	 * If this is just a wiring change, the two PTEs will be
	 * identical, so there's no need to update the page table.
	 */
	if (npte != opte) {
		boolean_t is_cached = pmap_is_current(pmap);

		*ptep = npte;
		if (is_cached) {
			/*
			 * We only need to frob the cache/tlb if this pmap
			 * is current
			 */
			PTE_SYNC(ptep);
			if (L1_IDX(va) != L1_IDX(vector_page) && 
			    l2pte_valid(npte)) {
				/*
				 * This mapping is likely to be accessed as
				 * soon as we return to userland. Fix up the
				 * L1 entry to avoid taking another
				 * page/domain fault.
				 */
				pd_entry_t *pl1pd, l1pd;

				pl1pd = &pmap->pm_l1->l1_kva[L1_IDX(va)];
				l1pd = l2b->l2b_phys | L1_C_DOM(pmap->pm_domain) |
				    L1_C_PROTO;
				if (*pl1pd != l1pd) {
					*pl1pd = l1pd;
					PTE_SYNC(pl1pd);
				}
			}
		}

		if (PV_BEEN_EXECD(oflags))
			pmap_tlb_flushID_SE(pmap, va);
		else if (PV_BEEN_REFD(oflags))
			pmap_tlb_flushD_SE(pmap, va);


		if (m)
			pmap_vac_me_harder(m, pmap, va);
	}
}

/*
 * Maps a sequence of resident pages belonging to the same object.
 * The sequence begins with the given page m_start.  This page is
 * mapped at the given virtual address start.  Each subsequent page is
 * mapped at a virtual address that is offset from start by the same
 * amount as the page is offset from m_start within the object.  The
 * last page in the sequence is the page with the largest offset from
 * m_start that can be mapped at a virtual address less than the given
 * virtual address end.  Not every virtual page between start and end
 * is mapped; only those for which a resident page exists with the
 * corresponding offset from m_start are mapped.
 */
void
pmap_enter_object(pmap_t pmap, vm_offset_t start, vm_offset_t end,
    vm_page_t m_start, vm_prot_t prot)
{
	vm_page_t m;
	vm_pindex_t diff, psize;

	psize = atop(end - start);
	m = m_start;
	PMAP_LOCK(pmap);
	while (m != NULL && (diff = m->pindex - m_start->pindex) < psize) {
		pmap_enter_locked(pmap, start + ptoa(diff), m, prot &
		    (VM_PROT_READ | VM_PROT_EXECUTE), FALSE, M_NOWAIT);
		m = TAILQ_NEXT(m, listq);
	}
 	PMAP_UNLOCK(pmap);
}

/*
 * this code makes some *MAJOR* assumptions:
 * 1. Current pmap & pmap exists.
 * 2. Not wired.
 * 3. Read access.
 * 4. No page table pages.
 * but is *MUCH* faster than pmap_enter...
 */

void
pmap_enter_quick(pmap_t pmap, vm_offset_t va, vm_page_t m, vm_prot_t prot)
{

 	PMAP_LOCK(pmap);
	pmap_enter_locked(pmap, va, m, prot & (VM_PROT_READ | VM_PROT_EXECUTE),
	    FALSE, M_NOWAIT);
 	PMAP_UNLOCK(pmap);
}

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_change_wiring(pmap_t pmap, vm_offset_t va, boolean_t wired)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	vm_page_t pg;

	vm_page_lock_queues();
 	PMAP_LOCK(pmap);
	l2b = pmap_get_l2_bucket(pmap, va);
	KASSERT(l2b, ("No l2b bucket in pmap_change_wiring"));
	ptep = &l2b->l2b_kva[l2pte_index(va)];
	pte = *ptep;
	pg = PHYS_TO_VM_PAGE(l2pte_pa(pte));
	if (pg) 
		pmap_modify_pv(pg, pmap, va, PVF_WIRED, wired);
	vm_page_unlock_queues();
 	PMAP_UNLOCK(pmap);
}


/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vm_offset_t dst_addr,
    vm_size_t len, vm_offset_t src_addr)
{
}


/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */
vm_paddr_t
pmap_extract(pmap_t pm, vm_offset_t va)
{
	struct l2_dtable *l2;
	pd_entry_t l1pd;
	pt_entry_t *ptep, pte;
	vm_paddr_t pa;
	u_int l1idx;
	l1idx = L1_IDX(va);

	PMAP_LOCK(pm);
	l1pd = pm->pm_l1->l1_kva[l1idx];
	if (l1pte_section_p(l1pd)) {
		/*
		 * These should only happen for pmap_kernel()
		 */
		KASSERT(pm == pmap_kernel(), ("huh"));
		/* XXX: what to do about the bits > 32 ? */
		if (l1pd & L1_S_SUPERSEC) 
			pa = (l1pd & L1_SUP_FRAME) | (va & L1_SUP_OFFSET);
		else
			pa = (l1pd & L1_S_FRAME) | (va & L1_S_OFFSET);
	} else {
		/*
		 * Note that we can't rely on the validity of the L1
		 * descriptor as an indication that a mapping exists.
		 * We have to look it up in the L2 dtable.
		 */
		l2 = pm->pm_l2[L2_IDX(l1idx)];

		if (l2 == NULL ||
		    (ptep = l2->l2_bucket[L2_BUCKET(l1idx)].l2b_kva) == NULL) {
			PMAP_UNLOCK(pm);
			return (0);
		}

		ptep = &ptep[l2pte_index(va)];
		pte = *ptep;

		if (pte == 0) {
			PMAP_UNLOCK(pm);
			return (0);
		}

		switch (pte & L2_TYPE_MASK) {
		case L2_TYPE_L:
			pa = (pte & L2_L_FRAME) | (va & L2_L_OFFSET);
			break;

		default:
			pa = (pte & L2_S_FRAME) | (va & L2_S_OFFSET);
			break;
		}
	}

	PMAP_UNLOCK(pm);
	return (pa);
}

/*
 * Atomically extract and hold the physical page with the given
 * pmap and virtual address pair if that mapping permits the given
 * protection.
 *
 */
vm_page_t
pmap_extract_and_hold(pmap_t pmap, vm_offset_t va, vm_prot_t prot)
{
	struct l2_dtable *l2;
	pd_entry_t l1pd;
	pt_entry_t *ptep, pte;
	vm_paddr_t pa;
	vm_page_t m = NULL;
	u_int l1idx;
	l1idx = L1_IDX(va);

	vm_page_lock_queues();
 	PMAP_LOCK(pmap);
	l1pd = pmap->pm_l1->l1_kva[l1idx];
	if (l1pte_section_p(l1pd)) {
		/*
		 * These should only happen for pmap_kernel()
		 */
		KASSERT(pmap == pmap_kernel(), ("huh"));
		/* XXX: what to do about the bits > 32 ? */
		if (l1pd & L1_S_SUPERSEC) 
			pa = (l1pd & L1_SUP_FRAME) | (va & L1_SUP_OFFSET);
		else
			pa = (l1pd & L1_S_FRAME) | (va & L1_S_OFFSET);
		if (l1pd & L1_S_PROT_W || (prot & VM_PROT_WRITE) == 0) {
			m = PHYS_TO_VM_PAGE(pa);
			vm_page_hold(m);
		}
			
	} else {
		/*
		 * Note that we can't rely on the validity of the L1
		 * descriptor as an indication that a mapping exists.
		 * We have to look it up in the L2 dtable.
		 */
		l2 = pmap->pm_l2[L2_IDX(l1idx)];

		if (l2 == NULL ||
		    (ptep = l2->l2_bucket[L2_BUCKET(l1idx)].l2b_kva) == NULL) {
		 	PMAP_UNLOCK(pmap);
			vm_page_unlock_queues();
			return (NULL);
		}

		ptep = &ptep[l2pte_index(va)];
		pte = *ptep;

		if (pte == 0) {
		 	PMAP_UNLOCK(pmap);
			vm_page_unlock_queues();
			return (NULL);
		}
		if (pte & L2_S_PROT_W || (prot & VM_PROT_WRITE) == 0) {
			switch (pte & L2_TYPE_MASK) {
			case L2_TYPE_L:
				pa = (pte & L2_L_FRAME) | (va & L2_L_OFFSET);
				break;
				
			default:
				pa = (pte & L2_S_FRAME) | (va & L2_S_OFFSET);
				break;
			}
			m = PHYS_TO_VM_PAGE(pa);
			vm_page_hold(m);
		}
	}

 	PMAP_UNLOCK(pmap);
	vm_page_unlock_queues();
	return (m);
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */

int
pmap_pinit(pmap_t pmap)
{
	PDEBUG(1, printf("pmap_pinit: pmap = %08x\n", (uint32_t) pmap));
	
	PMAP_LOCK_INIT(pmap);
	pmap_alloc_l1(pmap);
	bzero(pmap->pm_l2, sizeof(pmap->pm_l2));

	pmap->pm_count = 1;
	pmap->pm_active = 0;
		
	TAILQ_INIT(&pmap->pm_pvlist);
	bzero(&pmap->pm_stats, sizeof pmap->pm_stats);
	pmap->pm_stats.resident_count = 1;
	if (vector_page < KERNBASE) {
		pmap_enter(pmap, vector_page, 
		    VM_PROT_READ, PHYS_TO_VM_PAGE(systempage.pv_pa),
		    VM_PROT_READ, 1);
	} 
	return (1);
}


/***************************************************
 * page management routines.
 ***************************************************/


static void
pmap_free_pv_entry(pv_entry_t pv)
{
	pv_entry_count--;
	uma_zfree(pvzone, pv);
}


/*
 * get a new pv_entry, allocating a block from the system
 * when needed.
 * the memory allocation is performed bypassing the malloc code
 * because of the possibility of allocations at interrupt time.
 */
static pv_entry_t
pmap_get_pv_entry(void)
{
	pv_entry_t ret_value;
	
	pv_entry_count++;
	if (pv_entry_count > pv_entry_high_water)
		pagedaemon_wakeup();
	ret_value = uma_zalloc(pvzone, M_NOWAIT);
	return ret_value;
}


/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
#define  PMAP_REMOVE_CLEAN_LIST_SIZE     3
void
pmap_remove(pmap_t pm, vm_offset_t sva, vm_offset_t eva)
{
	struct l2_bucket *l2b;
	vm_offset_t next_bucket;
	pt_entry_t *ptep;
	u_int cleanlist_idx, total, cnt;
	struct {
		vm_offset_t va;
		pt_entry_t *pte;
	} cleanlist[PMAP_REMOVE_CLEAN_LIST_SIZE];
	u_int mappings, is_exec, is_refd;
	int flushall = 0;


	/*
	 * we lock in the pmap => pv_head direction
	 */

	vm_page_lock_queues();
	PMAP_LOCK(pm);
	if (!pmap_is_current(pm)) {
		cleanlist_idx = PMAP_REMOVE_CLEAN_LIST_SIZE + 1;
	} else
		cleanlist_idx = 0;

	total = 0;
	while (sva < eva) {
		/*
		 * Do one L2 bucket's worth at a time.
		 */
		next_bucket = L2_NEXT_BUCKET(sva);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pm, sva);
		if (l2b == NULL) {
			sva = next_bucket;
			continue;
		}

		ptep = &l2b->l2b_kva[l2pte_index(sva)];
		mappings = 0;

		while (sva < next_bucket) {
			struct vm_page *pg;
			pt_entry_t pte;
			vm_paddr_t pa;

			pte = *ptep;

			if (pte == 0) {
				/*
				 * Nothing here, move along
				 */
				sva += PAGE_SIZE;
				ptep++;
				continue;
			}

			pm->pm_stats.resident_count--;
			pa = l2pte_pa(pte);
			is_exec = 0;
			is_refd = 1;

			/*
			 * Update flags. In a number of circumstances,
			 * we could cluster a lot of these and do a
			 * number of sequential pages in one go.
			 */
			if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
				struct pv_entry *pve;

				pve = pmap_remove_pv(pg, pm, sva);
				if (pve) {
					is_exec = PV_BEEN_EXECD(pve->pv_flags);
					is_refd = PV_BEEN_REFD(pve->pv_flags);
					pmap_free_pv_entry(pve);
				}
			}

			if (!l2pte_valid(pte)) {
				*ptep = 0;
				PTE_SYNC_CURRENT(pm, ptep);
				sva += PAGE_SIZE;
				ptep++;
				mappings++;
				continue;
			}

			if (cleanlist_idx < PMAP_REMOVE_CLEAN_LIST_SIZE) {
				/* Add to the clean list. */
				cleanlist[cleanlist_idx].pte = ptep;
				cleanlist[cleanlist_idx].va =
				    sva | (is_exec & 1);
				cleanlist_idx++;
			} else
			if (cleanlist_idx == PMAP_REMOVE_CLEAN_LIST_SIZE) {
				/* Nuke everything if needed. */
				pmap_idcache_wbinv_all(pm);
				pmap_tlb_flushID(pm);

				/*
				 * Roll back the previous PTE list,
				 * and zero out the current PTE.
				 */
				for (cnt = 0;
				     cnt < PMAP_REMOVE_CLEAN_LIST_SIZE; cnt++) {
					*cleanlist[cnt].pte = 0;
				}
				*ptep = 0;
				PTE_SYNC(ptep);
				cleanlist_idx++;
				flushall = 1;
			} else {
				*ptep = 0;
				PTE_SYNC(ptep);
					if (is_exec)
						pmap_tlb_flushID_SE(pm, sva);
					else
					if (is_refd)
						pmap_tlb_flushD_SE(pm, sva);
			}

			sva += PAGE_SIZE;
			ptep++;
			mappings++;
		}

		/*
		 * Deal with any left overs
		 */
		if (cleanlist_idx <= PMAP_REMOVE_CLEAN_LIST_SIZE) {
			total += cleanlist_idx;
			for (cnt = 0; cnt < cleanlist_idx; cnt++) {
				vm_offset_t clva =
				    cleanlist[cnt].va & ~1;
				if (cleanlist[cnt].va & 1) {
					pmap_idcache_wbinv_range(pm,
					    clva, PAGE_SIZE);
					pmap_tlb_flushID_SE(pm, clva);
				} else {
					pmap_dcache_wb_range(pm,
					    clva, PAGE_SIZE, TRUE,
					    FALSE);
					pmap_tlb_flushD_SE(pm, clva);
				}
				*cleanlist[cnt].pte = 0;
				PTE_SYNC_CURRENT(pm, cleanlist[cnt].pte);
			}

			if (total <= PMAP_REMOVE_CLEAN_LIST_SIZE)
				cleanlist_idx = 0;
			else {
				/*
				 * We are removing so much entries it's just
				 * easier to flush the whole cache.
				 */
				cleanlist_idx = PMAP_REMOVE_CLEAN_LIST_SIZE + 1;
				pmap_idcache_wbinv_all(pm);
				flushall = 1;
			}
		}

		pmap_free_l2_bucket(pm, l2b, mappings);
	}

	vm_page_unlock_queues();
	if (flushall)
		cpu_tlb_flushID();
 	PMAP_UNLOCK(pm);
}




/*
 * pmap_zero_page()
 * 
 * Zero a given physical page by mapping it at a page hook point.
 * In doing the zero page op, the page we zero is mapped cachable, as with
 * StrongARM accesses to non-cached pages are non-burst making writing
 * _any_ bulk data very slow.
 */
#if (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0 || defined(CPU_XSCALE_CORE3)
void
pmap_zero_page_generic(vm_paddr_t phys, int off, int size)
{
#ifdef ARM_USE_SMALL_ALLOC
	char *dstpg;
#endif

#ifdef DEBUG
	struct vm_page *pg = PHYS_TO_VM_PAGE(phys);

	if (pg->md.pvh_list != NULL)
		panic("pmap_zero_page: page has mappings");
#endif

	if (_arm_bzero && size >= _min_bzero_size &&
	    _arm_bzero((void *)(phys + off), size, IS_PHYSICAL) == 0)
		return;

#ifdef ARM_USE_SMALL_ALLOC
	dstpg = (char *)arm_ptovirt(phys);
	if (off || size != PAGE_SIZE) {
		bzero(dstpg + off, size);
		cpu_dcache_wbinv_range((vm_offset_t)(dstpg + off), size);
	} else {
		bzero_page((vm_offset_t)dstpg);
		cpu_dcache_wbinv_range((vm_offset_t)dstpg, PAGE_SIZE);
	}
#else

	mtx_lock(&cmtx);
	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*cdst_pte = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	if (off || size != PAGE_SIZE) {
		bzero((void *)(cdstp + off), size);
		cpu_dcache_wbinv_range(cdstp + off, size);
	} else {
		bzero_page(cdstp);
		cpu_dcache_wbinv_range(cdstp, PAGE_SIZE);
	}
	mtx_unlock(&cmtx);
#endif
}
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0 */

#if ARM_MMU_XSCALE == 1
void
pmap_zero_page_xscale(vm_paddr_t phys, int off, int size)
{
#ifdef ARM_USE_SMALL_ALLOC
	char *dstpg;
#endif

	if (_arm_bzero && size >= _min_bzero_size &&
	    _arm_bzero((void *)(phys + off), size, IS_PHYSICAL) == 0)
		return;
#ifdef ARM_USE_SMALL_ALLOC
	dstpg = (char *)arm_ptovirt(phys);
	if (off || size != PAGE_SIZE) {
		bzero(dstpg + off, size);
		cpu_dcache_wbinv_range((vm_offset_t)(dstpg + off), size);
	} else {
		bzero_page((vm_offset_t)dstpg);
		cpu_dcache_wbinv_range((vm_offset_t)dstpg, PAGE_SIZE);
	}
#else
	mtx_lock(&cmtx);
	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*cdst_pte = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) |
	    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	if (off || size != PAGE_SIZE)
		bzero((void *)(cdstp + off), size);
	else
		bzero_page(cdstp);
	mtx_unlock(&cmtx);
	xscale_cache_clean_minidata();
#endif
}

/*
 * Change the PTEs for the specified kernel mappings such that they
 * will use the mini data cache instead of the main data cache.
 */
void
pmap_use_minicache(vm_offset_t va, vm_size_t size)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, *sptep, pte;
	vm_offset_t next_bucket, eva;

#if (ARM_NMMUS > 1) || defined(CPU_XSCALE_CORE3)
	if (xscale_use_minidata == 0)
		return;
#endif

	eva = va + size;

	while (va < eva) {
		next_bucket = L2_NEXT_BUCKET(va);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pmap_kernel(), va);

		sptep = ptep = &l2b->l2b_kva[l2pte_index(va)];

		while (va < next_bucket) {
			pte = *ptep;
			if (!l2pte_minidata(pte)) {
				cpu_dcache_wbinv_range(va, PAGE_SIZE);
				cpu_tlb_flushD_SE(va);
				*ptep = pte & ~L2_B;
			}
			ptep++;
			va += PAGE_SIZE;
		}
		PTE_SYNC_RANGE(sptep, (u_int)(ptep - sptep));
	}
	cpu_cpwait();
}
#endif /* ARM_MMU_XSCALE == 1 */

/*
 *	pmap_zero_page zeros the specified hardware page by mapping 
 *	the page into KVM and using bzero to clear its contents.
 */
void
pmap_zero_page(vm_page_t m)
{
	pmap_zero_page_func(VM_PAGE_TO_PHYS(m), 0, PAGE_SIZE);
}


/*
 *	pmap_zero_page_area zeros the specified hardware page by mapping 
 *	the page into KVM and using bzero to clear its contents.
 *
 *	off and size may not cover an area beyond a single hardware page.
 */
void
pmap_zero_page_area(vm_page_t m, int off, int size)
{

	pmap_zero_page_func(VM_PAGE_TO_PHYS(m), off, size);
}


/*
 *	pmap_zero_page_idle zeros the specified hardware page by mapping 
 *	the page into KVM and using bzero to clear its contents.  This
 *	is intended to be called from the vm_pagezero process only and
 *	outside of Giant.
 */
void
pmap_zero_page_idle(vm_page_t m)
{

	pmap_zero_page(m);
}

#if 0
/*
 * pmap_clean_page()
 *
 * This is a local function used to work out the best strategy to clean
 * a single page referenced by its entry in the PV table. It's used by
 * pmap_copy_page, pmap_zero page and maybe some others later on.
 *
 * Its policy is effectively:
 *  o If there are no mappings, we don't bother doing anything with the cache.
 *  o If there is one mapping, we clean just that page.
 *  o If there are multiple mappings, we clean the entire cache.
 *
 * So that some functions can be further optimised, it returns 0 if it didn't
 * clean the entire cache, or 1 if it did.
 *
 * XXX One bug in this routine is that if the pv_entry has a single page
 * mapped at 0x00000000 a whole cache clean will be performed rather than
 * just the 1 page. Since this should not occur in everyday use and if it does
 * it will just result in not the most efficient clean for the page.
 */
static int
pmap_clean_page(struct pv_entry *pv, boolean_t is_src)
{
	pmap_t pm, pm_to_clean = NULL;
	struct pv_entry *npv;
	u_int cache_needs_cleaning = 0;
	u_int flags = 0;
	vm_offset_t page_to_clean = 0;

	if (pv == NULL) {
		/* nothing mapped in so nothing to flush */
		return (0);
	}

	/*
	 * Since we flush the cache each time we change to a different
	 * user vmspace, we only need to flush the page if it is in the
	 * current pmap.
	 */
	if (curthread)
		pm = vmspace_pmap(curproc->p_vmspace);
	else
		pm = pmap_kernel();

	for (npv = pv; npv; npv = TAILQ_NEXT(npv, pv_list)) {
		if (npv->pv_pmap == pmap_kernel() || npv->pv_pmap == pm) {
			flags |= npv->pv_flags;
			/*
			 * The page is mapped non-cacheable in 
			 * this map.  No need to flush the cache.
			 */
			if (npv->pv_flags & PVF_NC) {
#ifdef DIAGNOSTIC
				if (cache_needs_cleaning)
					panic("pmap_clean_page: "
					    "cache inconsistency");
#endif
				break;
			} else if (is_src && (npv->pv_flags & PVF_WRITE) == 0)
				continue;
			if (cache_needs_cleaning) {
				page_to_clean = 0;
				break;
			} else {
				page_to_clean = npv->pv_va;
				pm_to_clean = npv->pv_pmap;
			}
			cache_needs_cleaning = 1;
		}
	}
	if (page_to_clean) {
		if (PV_BEEN_EXECD(flags))
			pmap_idcache_wbinv_range(pm_to_clean, page_to_clean,
			    PAGE_SIZE);
		else
			pmap_dcache_wb_range(pm_to_clean, page_to_clean,
			    PAGE_SIZE, !is_src, (flags & PVF_WRITE) == 0);
	} else if (cache_needs_cleaning) {
		if (PV_BEEN_EXECD(flags))
			pmap_idcache_wbinv_all(pm);
		else
			pmap_dcache_wbinv_all(pm);
		return (1);
	}
	return (0);
}
#endif

/*
 *	pmap_copy_page copies the specified (machine independent)
 *	page by mapping the page into virtual memory and using
 *	bcopy to copy the page, one machine dependent page at a
 *	time.
 */

/*
 * pmap_copy_page()
 *
 * Copy one physical page into another, by mapping the pages into
 * hook points. The same comment regarding cachability as in
 * pmap_zero_page also applies here.
 */
#if  (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0 || defined (CPU_XSCALE_CORE3)
void
pmap_copy_page_generic(vm_paddr_t src, vm_paddr_t dst)
{
#if 0
	struct vm_page *src_pg = PHYS_TO_VM_PAGE(src);
#endif
#ifdef DEBUG
	struct vm_page *dst_pg = PHYS_TO_VM_PAGE(dst);

	if (dst_pg->md.pvh_list != NULL)
		panic("pmap_copy_page: dst page has mappings");
#endif


	/*
	 * Clean the source page.  Hold the source page's lock for
	 * the duration of the copy so that no other mappings can
	 * be created while we have a potentially aliased mapping.
	 */
#if 0
	/*
	 * XXX: Not needed while we call cpu_dcache_wbinv_all() in
	 * pmap_copy_page().
	 */
	(void) pmap_clean_page(TAILQ_FIRST(&src_pg->md.pv_list), TRUE);
#endif
	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	mtx_lock(&cmtx);
	*csrc_pte = L2_S_PROTO | src |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) | pte_l2_s_cache_mode;
	PTE_SYNC(csrc_pte);
	*cdst_pte = L2_S_PROTO | dst |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(csrcp);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bcopy_page(csrcp, cdstp);
	mtx_unlock(&cmtx);
	cpu_dcache_inv_range(csrcp, PAGE_SIZE);
	cpu_dcache_wbinv_range(cdstp, PAGE_SIZE);
}
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0 */

#if ARM_MMU_XSCALE == 1
void
pmap_copy_page_xscale(vm_paddr_t src, vm_paddr_t dst)
{
#if 0
	/* XXX: Only needed for pmap_clean_page(), which is commented out. */
	struct vm_page *src_pg = PHYS_TO_VM_PAGE(src);
#endif
#ifdef DEBUG
	struct vm_page *dst_pg = PHYS_TO_VM_PAGE(dst);

	if (dst_pg->md.pvh_list != NULL)
		panic("pmap_copy_page: dst page has mappings");
#endif


	/*
	 * Clean the source page.  Hold the source page's lock for
	 * the duration of the copy so that no other mappings can
	 * be created while we have a potentially aliased mapping.
	 */
#if 0
	/*
	 * XXX: Not needed while we call cpu_dcache_wbinv_all() in
	 * pmap_copy_page().
	 */
	(void) pmap_clean_page(TAILQ_FIRST(&src_pg->md.pv_list), TRUE);
#endif
	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	mtx_lock(&cmtx);
	*csrc_pte = L2_S_PROTO | src |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) |
	    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(csrc_pte);
	*cdst_pte = L2_S_PROTO | dst |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) |
	    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(csrcp);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bcopy_page(csrcp, cdstp);
	mtx_unlock(&cmtx);
	xscale_cache_clean_minidata();
}
#endif /* ARM_MMU_XSCALE == 1 */

void
pmap_copy_page(vm_page_t src, vm_page_t dst)
{
#ifdef ARM_USE_SMALL_ALLOC
	vm_offset_t srcpg, dstpg;
#endif

	cpu_dcache_wbinv_all();
	if (_arm_memcpy && PAGE_SIZE >= _min_memcpy_size &&
	    _arm_memcpy((void *)VM_PAGE_TO_PHYS(dst), 
	    (void *)VM_PAGE_TO_PHYS(src), PAGE_SIZE, IS_PHYSICAL) == 0)
		return;
#ifdef ARM_USE_SMALL_ALLOC
	srcpg = arm_ptovirt(VM_PAGE_TO_PHYS(src));
	dstpg = arm_ptovirt(VM_PAGE_TO_PHYS(dst));
	bcopy_page(srcpg, dstpg);
	cpu_dcache_wbinv_range(dstpg, PAGE_SIZE);
#else
	pmap_copy_page_func(VM_PAGE_TO_PHYS(src), VM_PAGE_TO_PHYS(dst));
#endif
}




/*
 * this routine returns true if a physical page resides
 * in the given pmap.
 */
boolean_t
pmap_page_exists_quick(pmap_t pmap, vm_page_t m)
{
	pv_entry_t pv;
	int loops = 0;
	
	if (m->flags & PG_FICTITIOUS)
		return (FALSE);
		
	/*
	 * Not found, check current mappings returning immediately
	 */
	for (pv = TAILQ_FIRST(&m->md.pv_list);
	    pv;
	    pv = TAILQ_NEXT(pv, pv_list)) {
	    	if (pv->pv_pmap == pmap) {
	    		return (TRUE);
	    	}
		loops++;
		if (loops >= 16)
			break;
	}
	return (FALSE);
}

/*
 *	pmap_page_wired_mappings:
 *
 *	Return the number of managed mappings to the given physical page
 *	that are wired.
 */
int
pmap_page_wired_mappings(vm_page_t m)
{
	pv_entry_t pv;
	int count;

	count = 0;
	if ((m->flags & PG_FICTITIOUS) != 0)
		return (count);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	TAILQ_FOREACH(pv, &m->md.pv_list, pv_list)
		if ((pv->pv_flags & PVF_WIRED) != 0)
			count++;
	return (count);
}

/*
 *	pmap_ts_referenced:
 *
 *	Return the count of reference bits for a page, clearing all of them.
 */
int
pmap_ts_referenced(vm_page_t m)
{

	if (m->flags & PG_FICTITIOUS)
		return (0);
	return (pmap_clearbit(m, PVF_REF));
}


boolean_t
pmap_is_modified(vm_page_t m)
{

	if (m->md.pvh_attrs & PVF_MOD)
		return (TRUE);
	
	return(FALSE);
}


/*
 *	Clear the modify bits on the specified physical page.
 */
void
pmap_clear_modify(vm_page_t m)
{

	if (m->md.pvh_attrs & PVF_MOD)
		pmap_clearbit(m, PVF_MOD);
}


/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */
void
pmap_clear_reference(vm_page_t m)
{

	if (m->md.pvh_attrs & PVF_REF) 
		pmap_clearbit(m, PVF_REF);
}


/*
 * Clear the write and modified bits in each of the given page's mappings.
 */
void
pmap_remove_write(vm_page_t m)
{

	if (m->flags & PG_WRITEABLE)
		pmap_clearbit(m, PVF_WRITE);
}


/*
 * perform the pmap work for mincore
 */
int
pmap_mincore(pmap_t pmap, vm_offset_t addr)
{
	printf("pmap_mincore()\n");
	
	return (0);
}


vm_offset_t
pmap_addr_hint(vm_object_t obj, vm_offset_t addr, vm_size_t size)
{

	return(addr);
}


/*
 * Map a set of physical memory pages into the kernel virtual
 * address space. Return a pointer to where it is mapped. This
 * routine is intended to be used for mapping device memory,
 * NOT real memory.
 */
void *
pmap_mapdev(vm_offset_t pa, vm_size_t size)
{
	vm_offset_t va, tmpva, offset;
	
	offset = pa & PAGE_MASK;
	size = roundup(size, PAGE_SIZE);
	
	GIANT_REQUIRED;
	
	va = kmem_alloc_nofault(kernel_map, size);
	if (!va)
		panic("pmap_mapdev: Couldn't alloc kernel virtual memory");
	for (tmpva = va; size > 0;) {
		pmap_kenter_internal(tmpva, pa, 0);
		size -= PAGE_SIZE;
		tmpva += PAGE_SIZE;
		pa += PAGE_SIZE;
	}
	
	return ((void *)(va + offset));
}

#define BOOTSTRAP_DEBUG

/*
 * pmap_map_section:
 *
 *	Create a single section mapping.
 */
void
pmap_map_section(vm_offset_t l1pt, vm_offset_t va, vm_offset_t pa,
    int prot, int cache)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pd_entry_t fl;

	KASSERT(((va | pa) & L1_S_OFFSET) == 0, ("ouin2"));

	switch (cache) {
	case PTE_NOCACHE:
	default:
		fl = 0;
		break;

	case PTE_CACHE:
		fl = pte_l1_s_cache_mode;
		break;

	case PTE_PAGETABLE:
		fl = pte_l1_s_cache_mode_pt;
		break;
	}

	pde[va >> L1_S_SHIFT] = L1_S_PROTO | pa |
	    L1_S_PROT(PTE_KERNEL, prot) | fl | L1_S_DOM(PMAP_DOMAIN_KERNEL);
	PTE_SYNC(&pde[va >> L1_S_SHIFT]);

}

/*
 * pmap_link_l2pt:
 *
 *	Link the L2 page table specified by l2pv.pv_pa into the L1
 *	page table at the slot for "va".
 */
void
pmap_link_l2pt(vm_offset_t l1pt, vm_offset_t va, struct pv_addr *l2pv)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt, proto;
	u_int slot = va >> L1_S_SHIFT;

	proto = L1_S_DOM(PMAP_DOMAIN_KERNEL) | L1_C_PROTO;

#ifdef VERBOSE_INIT_ARM     
	printf("pmap_link_l2pt: pa=0x%x va=0x%x\n", l2pv->pv_pa, l2pv->pv_va);
#endif

	pde[slot + 0] = proto | (l2pv->pv_pa + 0x000);

	PTE_SYNC(&pde[slot]);

	SLIST_INSERT_HEAD(&kernel_pt_list, l2pv, pv_list);

	
}

/*
 * pmap_map_entry
 *
 * 	Create a single page mapping.
 */
void
pmap_map_entry(vm_offset_t l1pt, vm_offset_t va, vm_offset_t pa, int prot,
    int cache)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pt_entry_t fl;
	pt_entry_t *pte;

	KASSERT(((va | pa) & PAGE_MASK) == 0, ("ouin"));

	switch (cache) {
	case PTE_NOCACHE:
	default:
		fl = 0;
		break;

	case PTE_CACHE:
		fl = pte_l2_s_cache_mode;
		break;

	case PTE_PAGETABLE:
		fl = pte_l2_s_cache_mode_pt;
		break;
	}

	if ((pde[va >> L1_S_SHIFT] & L1_TYPE_MASK) != L1_TYPE_C)
		panic("pmap_map_entry: no L2 table for VA 0x%08x", va);

	pte = (pt_entry_t *) kernel_pt_lookup(pde[L1_IDX(va)] & L1_C_ADDR_MASK);

	if (pte == NULL)
		panic("pmap_map_entry: can't find L2 table for VA 0x%08x", va);

	pte[l2pte_index(va)] =
	    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot) | fl;
	PTE_SYNC(&pte[l2pte_index(va)]);
}

/*
 * pmap_map_chunk:
 *
 *	Map a chunk of memory using the most efficient mappings
 *	possible (section. large page, small page) into the
 *	provided L1 and L2 tables at the specified virtual address.
 */
vm_size_t
pmap_map_chunk(vm_offset_t l1pt, vm_offset_t va, vm_offset_t pa,
    vm_size_t size, int prot, int cache)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pt_entry_t *pte, f1, f2s, f2l;
	vm_size_t resid;  
	int i;

	resid = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

	if (l1pt == 0)
		panic("pmap_map_chunk: no L1 table provided");

#ifdef VERBOSE_INIT_ARM     
	printf("pmap_map_chunk: pa=0x%x va=0x%x size=0x%x resid=0x%x "
	    "prot=0x%x cache=%d\n", pa, va, size, resid, prot, cache);
#endif

	switch (cache) {
	case PTE_NOCACHE:
	default:
		f1 = 0;
		f2l = 0;
		f2s = 0;
		break;

	case PTE_CACHE:
		f1 = pte_l1_s_cache_mode;
		f2l = pte_l2_l_cache_mode;
		f2s = pte_l2_s_cache_mode;
		break;

	case PTE_PAGETABLE:
		f1 = pte_l1_s_cache_mode_pt;
		f2l = pte_l2_l_cache_mode_pt;
		f2s = pte_l2_s_cache_mode_pt;
		break;
	}

	size = resid;

	while (resid > 0) {
		/* See if we can use a section mapping. */
		if (L1_S_MAPPABLE_P(va, pa, resid)) {
#ifdef VERBOSE_INIT_ARM
			printf("S");
#endif
			pde[va >> L1_S_SHIFT] = L1_S_PROTO | pa |
			    L1_S_PROT(PTE_KERNEL, prot) | f1 |
			    L1_S_DOM(PMAP_DOMAIN_KERNEL);
			PTE_SYNC(&pde[va >> L1_S_SHIFT]);
			va += L1_S_SIZE;
			pa += L1_S_SIZE;
			resid -= L1_S_SIZE;
			continue;
		}

		/*
		 * Ok, we're going to use an L2 table.  Make sure
		 * one is actually in the corresponding L1 slot
		 * for the current VA.
		 */
		if ((pde[va >> L1_S_SHIFT] & L1_TYPE_MASK) != L1_TYPE_C)
			panic("pmap_map_chunk: no L2 table for VA 0x%08x", va);

		pte = (pt_entry_t *) kernel_pt_lookup(
		    pde[L1_IDX(va)] & L1_C_ADDR_MASK);
		if (pte == NULL)
			panic("pmap_map_chunk: can't find L2 table for VA"
			    "0x%08x", va);
		/* See if we can use a L2 large page mapping. */
		if (L2_L_MAPPABLE_P(va, pa, resid)) {
#ifdef VERBOSE_INIT_ARM
			printf("L");
#endif
			for (i = 0; i < 16; i++) {
				pte[l2pte_index(va) + i] =
				    L2_L_PROTO | pa |
				    L2_L_PROT(PTE_KERNEL, prot) | f2l;
				PTE_SYNC(&pte[l2pte_index(va) + i]);
			}
			va += L2_L_SIZE;
			pa += L2_L_SIZE;
			resid -= L2_L_SIZE;
			continue;
		}

		/* Use a small page mapping. */
#ifdef VERBOSE_INIT_ARM
		printf("P");
#endif
		pte[l2pte_index(va)] =
		    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot) | f2s;
		PTE_SYNC(&pte[l2pte_index(va)]);
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		resid -= PAGE_SIZE;
	}
#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif
	return (size);

}

/********************** Static device map routines ***************************/

static const struct pmap_devmap *pmap_devmap_table;

/*
 * Register the devmap table.  This is provided in case early console
 * initialization needs to register mappings created by bootstrap code
 * before pmap_devmap_bootstrap() is called.
 */
void
pmap_devmap_register(const struct pmap_devmap *table)
{

	pmap_devmap_table = table;
}

/*
 * Map all of the static regions in the devmap table, and remember
 * the devmap table so other parts of the kernel can look up entries
 * later.
 */
void
pmap_devmap_bootstrap(vm_offset_t l1pt, const struct pmap_devmap *table)
{
	int i;

	pmap_devmap_table = table;

	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
#ifdef VERBOSE_INIT_ARM
		printf("devmap: %08x -> %08x @ %08x\n",
		    pmap_devmap_table[i].pd_pa,
		    pmap_devmap_table[i].pd_pa +
			pmap_devmap_table[i].pd_size - 1,
		    pmap_devmap_table[i].pd_va);
#endif
		pmap_map_chunk(l1pt, pmap_devmap_table[i].pd_va,
		    pmap_devmap_table[i].pd_pa,
		    pmap_devmap_table[i].pd_size,
		    pmap_devmap_table[i].pd_prot,
		    pmap_devmap_table[i].pd_cache);
	}
}

const struct pmap_devmap *
pmap_devmap_find_pa(vm_paddr_t pa, vm_size_t size)
{
	int i;

	if (pmap_devmap_table == NULL)
		return (NULL);

	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if (pa >= pmap_devmap_table[i].pd_pa &&
		    pa + size <= pmap_devmap_table[i].pd_pa +
				 pmap_devmap_table[i].pd_size)
			return (&pmap_devmap_table[i]);
	}

	return (NULL);
}

const struct pmap_devmap *
pmap_devmap_find_va(vm_offset_t va, vm_size_t size)
{
	int i;

	if (pmap_devmap_table == NULL)
		return (NULL);

	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if (va >= pmap_devmap_table[i].pd_va &&
		    va + size <= pmap_devmap_table[i].pd_va +
				 pmap_devmap_table[i].pd_size)
			return (&pmap_devmap_table[i]);
	}

	return (NULL);
}

