/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD AND 4-Clause-BSD
 *
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com> of Allegro Networks, Inc.
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
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $NetBSD: pmap.c,v 1.28 2000/03/26 20:42:36 kleink Exp $
 */
/*-
 * Copyright (C) 2001 Benno Rice.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY Benno Rice ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/*
 * Native 64-bit page table operations for running without a hypervisor.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/rwlock.h>
#include <sys/endian.h>

#include <sys/kdb.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_extern.h>
#include <vm/vm_pageout.h>

#include <machine/cpu.h>
#include <machine/hid.h>
#include <machine/md_var.h>
#include <machine/mmuvar.h>

#include "mmu_oea64.h"
#include "mmu_if.h"
#include "moea64_if.h"

#define	PTESYNC()	__asm __volatile("ptesync");
#define	TLBSYNC()	__asm __volatile("tlbsync; ptesync");
#define	SYNC()		__asm __volatile("sync");
#define	EIEIO()		__asm __volatile("eieio");

#define	VSID_HASH_MASK	0x0000007fffffffffULL

/* POWER9 only permits a 64k partition table size. */
#define	PART_SIZE	0x10000

static __inline void
TLBIE(uint64_t vpn)
{
	vpn <<= ADDR_PIDX_SHFT;

	__asm __volatile("tlbie %0" :: "r"(vpn) : "memory");
	__asm __volatile("eieio; tlbsync; ptesync" ::: "memory");
}

#define DISABLE_TRANS(msr)	msr = mfmsr(); mtmsr(msr & ~PSL_DR)
#define ENABLE_TRANS(msr)	mtmsr(msr)

/*
 * PTEG data.
 */
static volatile struct pate *isa3_part_table;
static volatile struct lpte *isa3_hashtb_pteg_table;
static struct rwlock isa3_hashtb_eviction_lock;

/*
 * PTE calls.
 */
static int	isa3_hashtb_pte_insert(mmu_t, struct pvo_entry *);
static int64_t	isa3_hashtb_pte_synch(mmu_t, struct pvo_entry *);
static int64_t	isa3_hashtb_pte_clear(mmu_t, struct pvo_entry *, uint64_t);
static int64_t	isa3_hashtb_pte_replace(mmu_t, struct pvo_entry *, int);
static int64_t	isa3_hashtb_pte_unset(mmu_t mmu, struct pvo_entry *);

/*
 * Utility routines.
 */
static void	isa3_hashtb_bootstrap(mmu_t mmup, 
		    vm_offset_t kernelstart, vm_offset_t kernelend);
static void	isa3_hashtb_cpu_bootstrap(mmu_t, int ap);
static void	tlbia(void);

static mmu_method_t isa3_hashtb_methods[] = {
	/* Internal interfaces */
	MMUMETHOD(mmu_bootstrap,	isa3_hashtb_bootstrap),
	MMUMETHOD(mmu_cpu_bootstrap,	isa3_hashtb_cpu_bootstrap),

	MMUMETHOD(moea64_pte_synch,	isa3_hashtb_pte_synch),
	MMUMETHOD(moea64_pte_clear,	isa3_hashtb_pte_clear),	
	MMUMETHOD(moea64_pte_unset,	isa3_hashtb_pte_unset),	
	MMUMETHOD(moea64_pte_replace,	isa3_hashtb_pte_replace),	
	MMUMETHOD(moea64_pte_insert,	isa3_hashtb_pte_insert),	

	{ 0, 0 }
};

MMU_DEF_INHERIT(isa3_mmu_native, MMU_TYPE_P9H, isa3_hashtb_methods,
    0, oea64_mmu);

static int64_t
isa3_hashtb_pte_synch(mmu_t mmu, struct pvo_entry *pvo)
{
	volatile struct lpte *pt = isa3_hashtb_pteg_table + pvo->pvo_pte.slot;
	struct lpte properpt;
	uint64_t ptelo;

	PMAP_LOCK_ASSERT(pvo->pvo_pmap, MA_OWNED);

	moea64_pte_from_pvo(pvo, &properpt);

	rw_rlock(&isa3_hashtb_eviction_lock);
	if ((be64toh(pt->pte_hi) & LPTE_AVPN_MASK) !=
	    (properpt.pte_hi & LPTE_AVPN_MASK)) {
		/* Evicted */
		rw_runlock(&isa3_hashtb_eviction_lock);
		return (-1);
	}
		
	PTESYNC();
	ptelo = be64toh(pt->pte_lo);

	rw_runlock(&isa3_hashtb_eviction_lock);
	
	return (ptelo & (LPTE_REF | LPTE_CHG));
}

static int64_t 
isa3_hashtb_pte_clear(mmu_t mmu, struct pvo_entry *pvo, uint64_t ptebit)
{
	volatile struct lpte *pt = isa3_hashtb_pteg_table + pvo->pvo_pte.slot;
	struct lpte properpt;
	uint64_t ptelo;

	PMAP_LOCK_ASSERT(pvo->pvo_pmap, MA_OWNED);

	moea64_pte_from_pvo(pvo, &properpt);

	rw_rlock(&isa3_hashtb_eviction_lock);
	if ((be64toh(pt->pte_hi) & LPTE_AVPN_MASK) !=
	    (properpt.pte_hi & LPTE_AVPN_MASK)) {
		/* Evicted */
		rw_runlock(&isa3_hashtb_eviction_lock);
		return (-1);
	}

	if (ptebit == LPTE_REF) {
		/* See "Resetting the Reference Bit" in arch manual */
		PTESYNC();
		/* 2-step here safe: precision is not guaranteed */
		ptelo = be64toh(pt->pte_lo);

		/* One-byte store to avoid touching the C bit */
		((volatile uint8_t *)(&pt->pte_lo))[6] =
#if BYTE_ORDER == BIG_ENDIAN
		    ((uint8_t *)(&properpt.pte_lo))[6];
#else
		    ((uint8_t *)(&properpt.pte_lo))[1];
#endif
		rw_runlock(&isa3_hashtb_eviction_lock);

		critical_enter();
		TLBIE(pvo->pvo_vpn);
		critical_exit();
	} else {
		rw_runlock(&isa3_hashtb_eviction_lock);
		ptelo = isa3_hashtb_pte_unset(mmu, pvo);
		isa3_hashtb_pte_insert(mmu, pvo);
	}

	return (ptelo & (LPTE_REF | LPTE_CHG));
}

static int64_t
isa3_hashtb_pte_unset(mmu_t mmu, struct pvo_entry *pvo)
{
	volatile struct lpte *pt = isa3_hashtb_pteg_table + pvo->pvo_pte.slot;
	struct lpte properpt;
	uint64_t ptelo;

	moea64_pte_from_pvo(pvo, &properpt);

	rw_rlock(&isa3_hashtb_eviction_lock);
	if ((be64toh(pt->pte_hi & LPTE_AVPN_MASK)) !=
	    (properpt.pte_hi & LPTE_AVPN_MASK)) {
		/* Evicted */
		moea64_pte_overflow--;
		rw_runlock(&isa3_hashtb_eviction_lock);
		return (-1);
	}

	/*
	 * Invalidate the pte, briefly locking it to collect RC bits. No
	 * atomics needed since this is protected against eviction by the lock.
	 */
	isync();
	critical_enter();
	pt->pte_hi = be64toh((pt->pte_hi & ~LPTE_VALID) | LPTE_LOCKED);
	PTESYNC();
	TLBIE(pvo->pvo_vpn);
	ptelo = be64toh(pt->pte_lo);
	*((volatile int32_t *)(&pt->pte_hi) + 1) = 0; /* Release lock */
	critical_exit();
	rw_runlock(&isa3_hashtb_eviction_lock);

	/* Keep statistics */
	moea64_pte_valid--;

	return (ptelo & (LPTE_CHG | LPTE_REF));
}

static int64_t
isa3_hashtb_pte_replace(mmu_t mmu, struct pvo_entry *pvo, int flags)
{
	volatile struct lpte *pt = isa3_hashtb_pteg_table + pvo->pvo_pte.slot;
	struct lpte properpt;
	int64_t ptelo;

	if (flags == 0) {
		/* Just some software bits changing. */
		moea64_pte_from_pvo(pvo, &properpt);

		rw_rlock(&isa3_hashtb_eviction_lock);
		if ((be64toh(pt->pte_hi) & LPTE_AVPN_MASK) !=
		    (properpt.pte_hi & LPTE_AVPN_MASK)) {
			rw_runlock(&isa3_hashtb_eviction_lock);
			return (-1);
		}
		pt->pte_hi = htobe64(properpt.pte_hi);
		ptelo = be64toh(pt->pte_lo);
		rw_runlock(&isa3_hashtb_eviction_lock);
	} else {
		/* Otherwise, need reinsertion and deletion */
		ptelo = isa3_hashtb_pte_unset(mmu, pvo);
		isa3_hashtb_pte_insert(mmu, pvo);
	}

	return (ptelo);
}

static void
isa3_hashtb_cpu_bootstrap(mmu_t mmup, int ap)
{
	int i = 0;
	struct slb *slb = PCPU_GET(aim.slb);
	register_t seg0;

	/*
	 * Initialize segment registers and MMU
	 */

	mtmsr(mfmsr() & ~PSL_DR & ~PSL_IR);

	switch (mfpvr() >> 16) {
	case IBMPOWER9:
		mtspr(SPR_HID0, mfspr(SPR_HID0) & ~HID0_RADIX);
		break;
	}

	/*
	 * Install kernel SLB entries
	 */

	__asm __volatile ("slbia");
	__asm __volatile ("slbmfee %0,%1; slbie %0;" : "=r"(seg0) :
	    "r"(0));

	for (i = 0; i < n_slbs; i++) {
		if (!(slb[i].slbe & SLBE_VALID))
			continue;

		__asm __volatile ("slbmte %0, %1" :: 
		    "r"(slb[i].slbv), "r"(slb[i].slbe)); 
	}

	/*
	 * Install page table
	 */

	mtspr(SPR_PTCR,
	    ((uintptr_t)isa3_part_table & ~DMAP_BASE_ADDRESS) |
	     flsl((PART_SIZE >> 12) - 1));
	tlbia();
}

static void
isa3_hashtb_bootstrap(mmu_t mmup, vm_offset_t kernelstart,
    vm_offset_t kernelend)
{
	vm_size_t	size;
	vm_offset_t	off;
	vm_paddr_t	pa;
	register_t	msr;

	moea64_early_bootstrap(mmup, kernelstart, kernelend);

	/*
	 * Allocate PTEG table.
	 */

	size = moea64_pteg_count * sizeof(struct lpteg);
	CTR2(KTR_PMAP, "moea64_bootstrap: %d PTEGs, %lu bytes", 
	    moea64_pteg_count, size);
	rw_init(&isa3_hashtb_eviction_lock, "pte eviction");

	/*
	 * We now need to allocate memory. This memory, to be allocated,
	 * has to reside in a page table. The page table we are about to
	 * allocate. We don't have BAT. So drop to data real mode for a minute
	 * as a measure of last resort. We do this a couple times.
	 */

	isa3_part_table =
	    (struct pate *)moea64_bootstrap_alloc(PART_SIZE, PART_SIZE);
	if (hw_direct_map)
		isa3_part_table = (struct pate *)PHYS_TO_DMAP(
		    (vm_offset_t)isa3_part_table);
	/*
	 * PTEG table must be aligned on a 256k boundary, but can be placed
	 * anywhere with that alignment on POWER ISA 3+ systems.
	 */
	isa3_hashtb_pteg_table = (struct lpte *)moea64_bootstrap_alloc(size, 
	    MAX(256*1024, size));
	if (hw_direct_map)
		isa3_hashtb_pteg_table =
		    (struct lpte *)PHYS_TO_DMAP((vm_offset_t)isa3_hashtb_pteg_table);
	DISABLE_TRANS(msr);
	bzero(__DEVOLATILE(void *, isa3_part_table), PART_SIZE);
	isa3_part_table[0].pagetab =
	    ((uintptr_t)isa3_hashtb_pteg_table & ~DMAP_BASE_ADDRESS) |
	    (uintptr_t)(flsl((moea64_pteg_count - 1) >> 11));
	bzero(__DEVOLATILE(void *, isa3_hashtb_pteg_table), moea64_pteg_count *
	    sizeof(struct lpteg));
	ENABLE_TRANS(msr);

	CTR1(KTR_PMAP, "moea64_bootstrap: PTEG table at %p", isa3_hashtb_pteg_table);

	moea64_mid_bootstrap(mmup, kernelstart, kernelend);

	/*
	 * Add a mapping for the page table itself if there is no direct map.
	 */
	if (!hw_direct_map) {
		size = moea64_pteg_count * sizeof(struct lpteg);
		off = (vm_offset_t)(isa3_hashtb_pteg_table);
		DISABLE_TRANS(msr);
		for (pa = off; pa < off + size; pa += PAGE_SIZE)
			pmap_kenter(pa, pa);
		ENABLE_TRANS(msr);
	}

	/* Bring up virtual memory */
	moea64_late_bootstrap(mmup, kernelstart, kernelend);
}

static void
tlbia(void)
{
	vm_offset_t i;

	i = 0xc00; /* IS = 11 */

	TLBSYNC();

	for (; i < 0x200000; i += 0x00001000) {
		__asm __volatile("tlbiel %0" :: "r"(i));
	}

	EIEIO();
	TLBSYNC();
}

static int
atomic_pte_lock(volatile struct lpte *pte, uint64_t bitmask, uint64_t *oldhi)
{
	int	ret;
	uint32_t oldhihalf;

	/*
	 * Note: in principle, if just the locked bit were set here, we
	 * could avoid needing the eviction lock. However, eviction occurs
	 * so rarely that it isn't worth bothering about in practice.
	 */

	__asm __volatile (
		"1:\tlwarx %1, 0, %3\n\t"	/* load old value */
		"and. %0,%1,%4\n\t"		/* check if any bits set */
		"bne 2f\n\t"			/* exit if any set */
		"stwcx. %5, 0, %3\n\t"      	/* attempt to store */
		"bne- 1b\n\t"			/* spin if failed */
		"li %0, 1\n\t"			/* success - retval = 1 */
		"b 3f\n\t"			/* we've succeeded */
		"2:\n\t"
		"stwcx. %1, 0, %3\n\t"       	/* clear reservation (74xx) */
		"li %0, 0\n\t"			/* failure - retval = 0 */
		"3:\n\t"
		: "=&r" (ret), "=&r"(oldhihalf), "=m" (pte->pte_hi)
		: "r" ((volatile char *)&pte->pte_hi + 4),
		  "r" ((uint32_t)bitmask), "r" ((uint32_t)LPTE_LOCKED),
		  "m" (pte->pte_hi)
		: "cr0", "cr1", "cr2", "memory");

	*oldhi = (pte->pte_hi & 0xffffffff00000000ULL) | oldhihalf;

	return (ret);
}

static uintptr_t
isa3_hashtb_insert_to_pteg(struct lpte *pvo_pt, uintptr_t slotbase,
    uint64_t mask)
{
	volatile struct lpte *pt;
	uint64_t oldptehi, va;
	uintptr_t k;
	int i, j;

	/* Start at a random slot */
	i = mftb() % 8;
	for (j = 0; j < 8; j++) {
		k = slotbase + (i + j) % 8;
		pt = &isa3_hashtb_pteg_table[k];
		/* Invalidate and seize lock only if no bits in mask set */
		if (atomic_pte_lock(pt, mask, &oldptehi)) /* Lock obtained */
			break;
	}

	if (j == 8)
		return (-1);

	if (oldptehi & LPTE_VALID) {
		KASSERT(!(oldptehi & LPTE_WIRED), ("Unmapped wired entry"));
		/*
		 * Need to invalidate old entry completely: see
		 * "Modifying a Page Table Entry". Need to reconstruct
		 * the virtual address for the outgoing entry to do that.
		 */
		if (oldptehi & LPTE_BIG)
			va = oldptehi >> moea64_large_page_shift;
		else
			va = oldptehi >> ADDR_PIDX_SHFT;
		if (oldptehi & LPTE_HID)
			va = (((k >> 3) ^ moea64_pteg_mask) ^ va) &
			    VSID_HASH_MASK;
		else
			va = ((k >> 3) ^ va) & VSID_HASH_MASK;
		va |= (oldptehi & LPTE_AVPN_MASK) <<
		    (ADDR_API_SHFT64 - ADDR_PIDX_SHFT);
		PTESYNC();
		TLBIE(va);
		moea64_pte_valid--;
		moea64_pte_overflow++;
	}

	/*
	 * Update the PTE as per "Adding a Page Table Entry". Lock is released
	 * by setting the high doubleworld.
	 */
	pt->pte_lo = htobe64(pvo_pt->pte_lo);
	EIEIO();
	pt->pte_hi = htobe64(pvo_pt->pte_hi);
	PTESYNC();

	/* Keep statistics */
	moea64_pte_valid++;

	return (k);
}

static int
isa3_hashtb_pte_insert(mmu_t mmu, struct pvo_entry *pvo)
{
	struct lpte insertpt;
	uintptr_t slot;

	/* Initialize PTE */
	moea64_pte_from_pvo(pvo, &insertpt);

	/* Make sure further insertion is locked out during evictions */
	rw_rlock(&isa3_hashtb_eviction_lock);

	/*
	 * First try primary hash.
	 */
	pvo->pvo_pte.slot &= ~7ULL; /* Base slot address */
	slot = isa3_hashtb_insert_to_pteg(&insertpt, pvo->pvo_pte.slot,
	    LPTE_VALID | LPTE_WIRED | LPTE_LOCKED);
	if (slot != -1) {
		rw_runlock(&isa3_hashtb_eviction_lock);
		pvo->pvo_pte.slot = slot;
		return (0);
	}

	/*
	 * Now try secondary hash.
	 */
	pvo->pvo_vaddr ^= PVO_HID;
	insertpt.pte_hi ^= LPTE_HID;
	pvo->pvo_pte.slot ^= (moea64_pteg_mask << 3);
	slot = isa3_hashtb_insert_to_pteg(&insertpt, pvo->pvo_pte.slot,
	    LPTE_VALID | LPTE_WIRED | LPTE_LOCKED);
	if (slot != -1) {
		rw_runlock(&isa3_hashtb_eviction_lock);
		pvo->pvo_pte.slot = slot;
		return (0);
	}

	/*
	 * Out of luck. Find a PTE to sacrifice.
	 */

	/* Lock out all insertions for a bit */
	if (!rw_try_upgrade(&isa3_hashtb_eviction_lock)) {
		rw_runlock(&isa3_hashtb_eviction_lock);
		rw_wlock(&isa3_hashtb_eviction_lock);
	}

	slot = isa3_hashtb_insert_to_pteg(&insertpt, pvo->pvo_pte.slot,
	    LPTE_WIRED | LPTE_LOCKED);
	if (slot != -1) {
		rw_wunlock(&isa3_hashtb_eviction_lock);
		pvo->pvo_pte.slot = slot;
		return (0);
	}

	/* Try other hash table. Now we're getting desperate... */
	pvo->pvo_vaddr ^= PVO_HID;
	insertpt.pte_hi ^= LPTE_HID;
	pvo->pvo_pte.slot ^= (moea64_pteg_mask << 3);
	slot = isa3_hashtb_insert_to_pteg(&insertpt, pvo->pvo_pte.slot,
	    LPTE_WIRED | LPTE_LOCKED);
	if (slot != -1) {
		rw_wunlock(&isa3_hashtb_eviction_lock);
		pvo->pvo_pte.slot = slot;
		return (0);
	}

	/* No freeable slots in either PTEG? We're hosed. */
	rw_wunlock(&isa3_hashtb_eviction_lock);
	panic("moea64_pte_insert: overflow");
	return (-1);
}

