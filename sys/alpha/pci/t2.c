/*
 * Copyright (c) 2000 Andrew Gallatin & Doug Rabson
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
 * Portions of this file were obtained from Compaq intellectual
 * property which was made available under the following copyright:
 *
 * *****************************************************************
 * *                                                               *
 * *    Copyright Compaq Computer Corporation, 2000                *
 * *                                                               *
 * *   Permission to use, copy, modify, distribute, and sell       *
 * *   this software and its documentation for any purpose is      *
 * *   hereby granted without fee, provided that the above         *
 * *   copyright notice appear in all copies and that both         *
 * *   that copyright notice and this permission notice appear     *
 * *   in supporting documentation, and that the name of           *
 * *   Compaq Computer Corporation not be used in advertising      *
 * *   or publicity pertaining to distribution of the software     *
 * *   without specific, written prior permission.  Compaq         *
 * *   makes no representations about the suitability of this      *
 * *   software for any purpose.  It is provided "AS IS"           *
 * *   without express or implied warranty.                        *
 * *                                                               *
 * *****************************************************************
 *
 * $FreeBSD$
 */

/*
 * T2 CBUS to PCI bridge
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/proc.h>
#include <sys/rman.h>
#include <sys/interrupt.h>

#include <alpha/pci/t2reg.h>
#include <alpha/pci/t2var.h>
#include <alpha/pci/pcibus.h>
#include <alpha/isa/isavar.h>
#include <machine/intr.h>
#include <machine/resource.h>
#include <machine/intrcnt.h>
#include <machine/cpuconf.h>
#include <machine/swiz.h>
#include <machine/sgmap.h>
#include <pci/pcivar.h>

#include <vm/vm.h>
#include <vm/vm_page.h>

#define KV(pa)			ALPHA_PHYS_TO_K0SEG(pa + sable_lynx_base)

vm_offset_t	sable_lynx_base = 0UL;

volatile t2_csr_t *t2_csr[2];
static int pci_int_type[2];

static devclass_t	t2_devclass;
static device_t		t2_0;		/* XXX only one for now */

struct t2_softc {
	int		junk;
};

#define T2_SOFTC(dev)	(struct t2_softc*) device_get_softc(dev)

static alpha_chipset_read_hae_t	t2_read_hae;
static alpha_chipset_write_hae_t t2_write_hae;

static alpha_chipset_t t2_chipset = {
	t2_read_hae,
	t2_write_hae,
};

static u_int32_t	t2_hae_mem[2];

#define REG1 (1UL << 24)

static u_int32_t
t2_set_hae_mem(void *arg, u_int32_t pa)
{
	int s; 
	u_int32_t msb;
	int hose;

	hose = (long)arg;

	if(pa >= REG1){
		msb = pa & 0xf8000000;
		pa -= msb;
		msb >>= 27;	/* t2 puts high bits in the bottom of the register */
		s = splhigh();
		if (msb != t2_hae_mem[hose]) {
			t2_hae_mem[hose] = msb;
			t2_csr[hose]->hae0_1 = t2_hae_mem[hose];
			alpha_mb();
			t2_hae_mem[hose] = t2_csr[hose]->hae0_1;
		}
		splx(s);
	}
	return pa;
}

static u_int64_t
t2_read_hae(void)
{
	return t2_hae_mem[0] << 27;
}

static void
t2_write_hae(u_int64_t hae)
{
	u_int32_t pa = hae;
	t2_set_hae_mem(0, pa);
}

static int t2_probe(device_t dev);
static int t2_attach(device_t dev);
static int t2_setup_intr(device_t dev, device_t child,
			    struct resource *irq, int flags,
			    void *intr, void *arg, void **cookiep);
static int t2_teardown_intr(device_t dev, device_t child,
			    struct resource *irq, void *cookie);
static void
t2_dispatch_intr(void *frame, unsigned long vector);
static void
t2_machine_check(unsigned long mces, struct trapframe *framep, 
		 unsigned long vector, unsigned long param);


static device_method_t t2_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,			t2_probe),
	DEVMETHOD(device_attach,		t2_attach),

	/* Bus interface */
	DEVMETHOD(bus_alloc_resource,		pci_alloc_resource),
	DEVMETHOD(bus_release_resource,		pci_release_resource),
	DEVMETHOD(bus_activate_resource, 	pci_activate_resource),
	DEVMETHOD(bus_deactivate_resource,	pci_deactivate_resource),
	DEVMETHOD(bus_setup_intr,		t2_setup_intr),
	DEVMETHOD(bus_teardown_intr,		t2_teardown_intr),

	{ 0, 0 }
};

static driver_t t2_driver = {
	"t2",
	t2_methods,
	sizeof(struct t2_softc),
};


#define T2_SGMAP_BASE		(8*1024*1024)
#define T2_SGMAP_SIZE		(8*1024*1024)

static void
t2_sgmap_invalidate(void)
{
	u_int64_t val;

	alpha_mb();
	val = REGVAL64(T2_IOCSR);
	val |= T2_IOCSRL_ITLB;
	REGVAL64(T2_IOCSR) = val;
	alpha_mb();
	alpha_mb();
	val = REGVAL64(T2_IOCSR);
	val &= ~T2_IOCSRL_ITLB;
	REGVAL64(T2_IOCSR) = val;
	alpha_mb();
	alpha_mb();
}

static void
t2_sgmap_map(void *arg, bus_addr_t ba, vm_offset_t pa)
{
	u_int64_t *sgtable = arg;
	int index = alpha_btop(ba - T2_SGMAP_BASE);

	if (pa) {
		if (pa > (1L<<32))
			panic("t2_sgmap_map: can't map address 0x%lx", pa);
		sgtable[index] = ((pa >> 13) << 1) | 1;
	} else {
		sgtable[index] = 0;
	}
	alpha_mb();
	t2_sgmap_invalidate();
}


static void
t2_init_sgmap(int h)
{
	void *sgtable;

	/*
	 * First setup Window 2 to map 8Mb to 16Mb with an
	 * sgmap. Allocate the map aligned to a 32 boundary.
	 *
	 *  bits 31..20 of WBASE represent the pci start address
	 *  (in units of 1Mb), and bits 11..0 represent the pci
	 *  end address
	 */
	t2_csr[h]->wbase2 = T2_WSIZE_8M|T2_WINDOW_ENABLE|T2_WINDOW_SG
	                     | ((T2_SGMAP_BASE >> 20) << 20)
	                     | ((T2_SGMAP_BASE + T2_SGMAP_SIZE) >> 20);
	t2_csr[h]->wmask2 = T2_WMASK_8M;
	alpha_mb();

	sgtable = contigmalloc(8192, M_DEVBUF, M_NOWAIT,
			       0, (1L<<34),
			       32*1024, (1L<<34));
	if (!sgtable)
		panic("t2_init_sgmap: can't allocate page table");

	t2_csr[h]->tbase2 =
	    (pmap_kextract((vm_offset_t) sgtable) >> T2_TBASE_SHIFT);

	chipset.sgmap = sgmap_map_create(T2_SGMAP_BASE,
					 T2_SGMAP_BASE + T2_SGMAP_SIZE,
					 t2_sgmap_map, sgtable);
}

static void
t2_csr_init(int h)
{
	/* 
	 * initialize the DMA windows
	 */
	t2_csr[h]->wbase1 = T2_WSIZE_1G|T2_WINDOW_ENABLE|T2_WINDOW_DIRECT|0x7ff;
	t2_csr[h]->wmask1 = T2_WMASK_1G;
	t2_csr[h]->tbase1 = 0x0;

	t2_csr[h]->wbase2 = 0x0;

	/* 
	 *  enable the PCI "Hole" for ISA devices which use memory in
	 *  the 512k - 1MB range
	 */
	t2_csr[h]->hbase = 1 << 13;
	t2_init_sgmap(0);

	/* initialize the HAEs */
	t2_csr[h]->hae0_1 = 0x0;
	alpha_mb();
	t2_csr[h]->hae0_2 = 0x0;
	alpha_mb();
	t2_csr[h]->hae0_3 = 0x0;
	alpha_mb();

}

/*
 * Perform basic chipset init/fixup.  Called by various early
 * consumers to ensure that the system will work before the 
 * bus methods are invoked.
 *
 */

void
t2_init()
{
	static int initted = 0;
	static struct swiz_space io_space, mem_space;

	if (initted) return;
	initted = 1;

	swiz_init_space(&io_space, KV(T2_PCI_SIO));
	swiz_init_space_hae(&mem_space, KV(T2_PCI_SPARSE),
			    t2_set_hae_mem, 0);

	busspace_isa_io = (kobj_t) &io_space;
	busspace_isa_mem = (kobj_t) &mem_space;

	chipset = t2_chipset;

}

static int
t2_probe(device_t dev)
{
	int h, t2_num_hoses = 1;
	device_t child;

	if (t2_0)
		return ENXIO;

	t2_0 = dev;
	device_set_desc(dev, "T2 Core Logic chipset"); 
	t2_csr[0] = (t2_csr_t *)
	    ALPHA_PHYS_TO_K0SEG(sable_lynx_base + PCI0_BASE);
	t2_csr[1] = (t2_csr_t *)
	    ALPHA_PHYS_TO_K0SEG(sable_lynx_base + PCI1_BASE);

	/* Look at the rev of the chip.  If the high bit is set in the
	 * rev field then we have either a T3 or a T4 chip, so use the
	 * new interrupt structure.  If it is clear, then we have a T2
	 * so use the old way */

	platform.mcheck_handler = t2_machine_check;

	if (((t2_csr[0]->iocsr) >> 35) & 1)
		pci_int_type[0] = 1;
	 else 
		pci_int_type[0] = 0;

	device_printf(dev, "using interrupt type %d on pci bus 0\n", 
	    pci_int_type[0]);

	if (!badaddr((void *)&t2_csr[1]->tlbbr, sizeof(long))) {
		pci_int_type[1] = 1; /* PCI1 always uses the new scheme */
		/* Clear any errors that the BADADDR probe may have caused */
		t2_csr[1]->cerr1 |= t2_csr[1]->cerr1;
		t2_csr[1]->pcierr1 |= t2_csr[1]->pcierr1;
		device_printf(dev, "found EXT_IO!!!!!\n");
		/* t2_num_hoses = 2; XXX not ready for this yet */
	}

	pci_init_resources();

	for (h = 0; h < t2_num_hoses; h++)
		t2_csr_init(h);

	
	child = device_add_child(dev, "pcib", 0);
	device_set_ivars(child, 0);

	return 0;
}

static int
t2_attach(device_t dev)
{
	t2_init();

	set_iointr(t2_dispatch_intr);
	platform.isa_setup_intr = t2_setup_intr;
	platform.isa_teardown_intr = t2_teardown_intr;

	snprintf(chipset_type, sizeof(chipset_type), "t2");

	bus_generic_attach(dev);

	return 0;
}

/*
 * Map pci slot & INTx pin to the ICIC interrupt value for our PCIs.
 */

static int 
t2_ICIC_slot_to_STDIO_irq(device_t bus, device_t dev, int pin)
{

	int ret_irq = 0;

	/*
	 * Return the interrupt pin number for the PCI slots.  
	 */

	/*
	 * Generate the proper interrupt conversion for the physical
	 * PCI slots (for both the primary PCI slots and those behind
	 * a PPB). 
	 */
	/*
	 * XXX This code is wrong; we need to determine the correct
	 *     swizzle for devices behind the onboard PCI:PCI bridge
	 *     and ensure that the generic bridge code doesn't try to
	 *     reroute them.
	 */
	if ((pci_get_slot(dev) >= 6) && (pci_get_slot(dev) <= 9)) {
		ret_irq = (32 + (4 * (pci_get_slot(dev) - 6))) +
		    (pin - 1) + (16 * pci_get_bus(dev));
		return (ret_irq);
	}

	/* Convert the NCR810A chip behind the PPB */
	if (pci_get_slot(dev) == 1) {
		ret_irq = 28;
		return (ret_irq);
	}

	/*
	 * Convert the NCR810A chip on the primary PCI bus or the
	 * TULIP chip behind the PPB.  There is no system that has
	 * both, so there really is no sharing going on although it
	 * looks like it. 
	 */
	if ((pci_get_slot(dev) == 4) || (pci_get_slot(dev) == 0)) {
		ret_irq = 24;
		return (ret_irq);
	}

	printf("ICIC invalid pci slot: 0x%x intpin: 0x%x bus num:0x%x\n", 
	       pci_get_slot(dev), pin, pci_get_bus(dev));
	return(-1);
}

/*
 * Map pci slot & INTx pin to STDIO's 8259 irq input value for PCI0.
 */

static int 
t2_pci0_slot_to_STDIO_irq(device_t bus, device_t dev, int pin)
{

	switch(pci_get_slot(dev)) {
	case 0:	/* ethernet (tulip) port */
		return(0x2);
	case 1:	/* scsi 810 */
		return(0x1);
	case 6:	/* optional slot 0 */
		switch (pin) {
		case 1: return(0x0);
		case 2: return(0x18);
		case 3: return(0x1a);
		case 4: return(0x1d);
		}
	case 7:	/* optional slot 1 */
		switch (pin) {
		case 1: return(0x4);
		case 2: return(0x19);
		case 3: return(0x1b);
		case 4: return(0x1e);
		}
	case 8:	/* optional slot 2 */
		switch (pin) {
		case 1: return(0x5);
		case 2: return(0x14);
		case 3: return(0x1c);
		case 4: return(0x1f);
		}
	default:	/* invalid slot */
		printf("PCI slot %d unknown\n", pci_get_slot(dev));
		return(-1);
	}
	printf("invalid pci0 intpin slot: 0x%x intpin: 0x%x\n", 
	       pci_get_slot(dev), pin);
	return (-1);
}

int
t2_intr_route(device_t bus, device_t dev, int pin)
{
	if (pci_int_type[0]) {
		return (t2_ICIC_slot_to_STDIO_irq(bus, dev, pin));
	} else {
		return (t2_pci0_slot_to_STDIO_irq(bus, dev, pin));
	}
}

/*
 * magical mystery table partly obtained from Linux
 * at least some of their values for PCI masks
 * were incorrect, and I've filled in my own extrapolations
 * XXX this needs more testers 
 */

unsigned long t2_shadow_mask = -1L;
static const char irq_to_mask[40] = {
	-1,  6, -1,  8, 15, 12,  7,  9,		/* ISA 0-7  */
	-1, 16, 17, 18,  3, -1, 21, 22, 	/* ISA 8-15 */
	-1, -1, -1, -1, -1, -1, -1, -1,		/* ?? EISA XXX */
	-1, -1, -1, -1, -1, -1, -1, -1,		/* ?? EISA XXX */
	 0,  1,  2,  3,  4,  5,  6,  7		/* PCI 0-7 XXX */
};


static void
t2_8259_disable_mask(int mask)
{
	t2_shadow_mask |= (1UL << mask);

	if (mask <= 7)
		outb(SLAVE0_ICU, t2_shadow_mask);
	else if (mask <= 15)
		outb(SLAVE1_ICU, t2_shadow_mask >> 8);
	else 
		outb(SLAVE2_ICU, t2_shadow_mask >> 16);
}

static void
t2_8259_enable_mask(int mask)
{
	t2_shadow_mask &= ~(1UL << mask);

	if (mask <= 7)
		outb(SLAVE0_ICU, t2_shadow_mask);
	else if (mask <= 15)
		outb(SLAVE1_ICU, t2_shadow_mask >> 8);
	else 
		outb(SLAVE2_ICU, t2_shadow_mask >> 16);
}


static void 
t2_eoi( int vector)
{
	int irq, hose;

	hose = (vector >= 0xC00);
	irq = (vector - 0x800) >> 4;

	if (pci_int_type[hose]) {

		/* New interrupt scheme.  Both PCI0 and PCI1 can use
		 * the same handler.  Dispatching interrupts with the
		 * IC IC chip is easy.  We simply write the vector
		 * address  register (var) on the T3/T4 (offset
		 * 0x480) with the IRQ  level (0 - 63) of what came in.  */
		t2_csr[hose]->var = (u_long) irq;
		alpha_mb();
		alpha_mb();
	} else {
		switch (irq) {
		case 0 ... 7:
			outb(SLAVE0_ICU-1, (0xe0 | (irq)));
			outb(MASTER_ICU-1, (0xe0 | 1));
			break;
		case 8 ... 15:
			outb(SLAVE1_ICU-1, (0xe0 | (irq - 8)));
			outb(MASTER_ICU-1, (0xe0 | 3));
			break;
		case 16 ... 24:
			outb(SLAVE2_ICU-1, (0xe0 | (irq - 16)));
			outb(MASTER_ICU-1, (0xe0 | 4));
			break;
		}	
	}
}

static void
t2_enable_vec(int vector)
{
	int irq, hose;
	u_long IC_mask, scratch;

	hose = (vector >= 0xC00);
	irq = (vector - 0x800) >> 4;

	if (pci_int_type[hose]) {

		/* Write the air register on the T3/T4 with the
		 * address of the IC IC masks register (offset 0x40) */
		t2_csr[hose]->air = 0x40;
		alpha_mb();
		scratch = t2_csr[hose]->air;	
		alpha_mb();
		IC_mask = t2_csr[hose]->dir;
		IC_mask &= ~(1L << ( (u_long) irq));
		t2_csr[hose]->dir = IC_mask;	
		alpha_mb();
		alpha_mb();
		/*
		 * EOI the interrupt we just enabled.
		 */
		t2_eoi(vector);
	} else {
		/* Old style 8259 (Gack!!!) interrupts */
		t2_8259_enable_mask(irq);
	}
}

static void
t2_disable_vec(int vector)
{
	int hose, irq;
	u_long scratch, IC_mask;

	hose = (vector >= 0xC00);
	irq =  (vector - 0x800) >> 4;

	if (pci_int_type[hose]) {

		/* Write the air register on the T3/T4 wioth the
		 * address of the IC IC masks register (offset 0x40) */

		t2_csr[hose]->air = 0x40;
		alpha_mb();
		scratch = t2_csr[hose]->air;	
		alpha_mb();
		/*
		 * Read the dir register to fetch the mask data, 'or' in the
		 * new disable bit, and write the data back.
		 */
		IC_mask = t2_csr[hose]->dir;
		IC_mask |= (1L << ( (u_long) irq));
		/* Set the disable bit */
		t2_csr[hose]->dir = IC_mask;	
		alpha_mb();
		alpha_mb();
	} else {
		/* Old style 8259 (Gack!!!) interrupts */
		t2_8259_disable_mask(irq);
	}
}


static int
t2_setup_intr(device_t dev, device_t child,
	       struct resource *irq, int flags,
	       void *intr, void *arg, void **cookiep)
{
	int error, vector, stdio_irq;
	const char *name;
	device_t bus, parent;

	name = device_get_nameunit(dev);
	stdio_irq = irq->r_start;
	if (strncmp(name, "eisa", 4) == 0) {
		if ((stdio_irq != 6 ) && (stdio_irq != 3 )) {
			stdio_irq = 
			    T2_EISA_IRQ_TO_STDIO_IRQ(stdio_irq);
		}
	} else if ((strncmp(name, "isa", 3)) == 0) {
		stdio_irq = irq_to_mask[stdio_irq];
	}

	parent = dev;
	do {
		bus = parent;
		parent = device_get_parent(bus);
	} while (parent && strncmp("t2", device_get_nameunit(parent), 2));

	if (parent && (device_get_unit(bus) != 0))
		vector = STDIO_PCI1_IRQ_TO_SCB_VECTOR(stdio_irq);
	else	
		vector = STDIO_PCI0_IRQ_TO_SCB_VECTOR(stdio_irq);

	error = rman_activate_resource(irq);
	if (error)
		return error;

	error = alpha_setup_intr(device_get_nameunit(child ? child : dev),
			vector, intr, arg, flags, cookiep,
			&intrcnt[irq->r_start], t2_disable_vec, t2_enable_vec);
	    
	if (error)
		return error;

	/* Enable interrupt */
	t2_enable_vec(vector);
	
	if (bootverbose != 0) 
		device_printf(child, 
		    "interrupting at T2 irq %d (stdio irq %d)\n",
		      (int) irq->r_start, stdio_irq);
	return 0;
}

static int
t2_teardown_intr(device_t dev, device_t child,
	       struct resource *irq, void *cookie)
{
	int mask;
	
	mask = irq_to_mask[irq->r_start];

	/* Disable interrupt */
	
	/* 
	 *  XXX this is totally broken! 
	 *  we don't have enough info to figure out where the interrupt 
	 *  came from if hose != 0 and pci_int_type[hose] != 0
	 *  We should probably carry around the vector someplace --
	 *  that would be enough to figure out the hose and the stdio irq
	 */

	t2_shadow_mask |= (1UL << mask);

	if (mask <= 7)
		outb(SLAVE0_ICU, t2_shadow_mask);
	else if (mask <= 15)
		outb(SLAVE1_ICU, t2_shadow_mask >> 8);
	else 
		outb(SLAVE2_ICU, t2_shadow_mask >> 16);

	alpha_teardown_intr(cookie);
	return rman_deactivate_resource(irq);
}



static void
t2_dispatch_intr(void *frame, unsigned long vector)
{
	alpha_dispatch_intr(frame, vector);
	t2_eoi(vector);
}

static void
t2_machine_check(unsigned long mces, struct trapframe *framep, 
    unsigned long vector, unsigned long param)
{
	int expected;

	expected = mc_expected;
	machine_check(mces, framep, vector, param);
	/* for some reason the alpha_pal_wrmces() doesn't clear all
	   pending machine checks & we may take another */
	mc_expected = expected;
}

DRIVER_MODULE(t2, root, t2_driver, t2_devclass, 0, 0);
