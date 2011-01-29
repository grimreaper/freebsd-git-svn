/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 */
/*-
 * Copyright (c) 2002 Benno Rice.
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
 *	from: @(#)isa.c	7.2 (Berkeley) 5/13/91
 *	form: src/sys/i386/isa/intr_machdep.c,v 1.57 2001/07/20
 *
 * $FreeBSD$
 */

#include "opt_isa.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/bus.h>
#include <sys/interrupt.h>
#include <sys/ktr.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/pcpu.h>
#include <sys/smp.h>
#include <sys/syslog.h>
#include <sys/vmmeter.h>
#include <sys/proc.h>

#include <machine/frame.h>
#include <machine/intr_machdep.h>
#include <machine/md_var.h>
#include <machine/smp.h>
#include <machine/trap.h>

#include "pic_if.h"

#define	MAX_STRAY_LOG	5

MALLOC_DEFINE(M_INTR, "intr", "interrupt handler data");

struct powerpc_intr {
	struct intr_event *event;
	long	*cntp;
	u_int	irq;
	device_t pic;
	u_int	intline;
	u_int	vector;
	u_int	cntindex;
	cpumask_t cpu;
	enum intr_trigger trig;
	enum intr_polarity pol;
};

struct pic {
	device_t dev;
	uint32_t node;
	u_int	irqs;
	u_int	ipis;
	int	base;
};

static u_int intrcnt_index = 0;
static struct mtx intr_table_lock;
static struct powerpc_intr *powerpc_intrs[INTR_VECTORS];
static struct pic piclist[MAX_PICS];
static u_int nvectors;		/* Allocated vectors */
static u_int npics;		/* PICs registered */
#ifdef DEV_ISA
static u_int nirqs = 16;	/* Allocated IRQS (ISA pre-allocated). */
#else
static u_int nirqs = 0;		/* Allocated IRQs. */
#endif
static u_int stray_count;

device_t root_pic;

#ifdef SMP
static void *ipi_cookie;
#endif

static void
intr_init(void *dummy __unused)
{

	mtx_init(&intr_table_lock, "intr sources lock", NULL, MTX_DEF);
}
SYSINIT(intr_init, SI_SUB_INTR, SI_ORDER_FIRST, intr_init, NULL);

#ifdef SMP
static void
smp_intr_init(void *dummy __unused)
{
	struct powerpc_intr *i;
	int vector;

	for (vector = 0; vector < nvectors; vector++) {
		i = powerpc_intrs[vector];
		if (i != NULL && i->pic == root_pic)
			PIC_BIND(i->pic, i->intline, i->cpu);
	}
}
SYSINIT(smp_intr_init, SI_SUB_SMP, SI_ORDER_ANY, smp_intr_init, NULL);
#endif

static void
intrcnt_setname(const char *name, int index)
{

	snprintf(intrnames + (MAXCOMLEN + 1) * index, MAXCOMLEN + 1, "%-*s",
	    MAXCOMLEN, name);
}

void
intrcnt_add(const char *name, u_long **countp)
{
	int idx;

	idx = atomic_fetchadd_int(&intrcnt_index, 1);
	*countp = &intrcnt[idx];
	intrcnt_setname(name, idx);
}

static struct powerpc_intr *
intr_lookup(u_int irq)
{
	char intrname[8];
	struct powerpc_intr *i, *iscan;
	int vector;

	mtx_lock(&intr_table_lock);
	for (vector = 0; vector < nvectors; vector++) {
		i = powerpc_intrs[vector];
		if (i != NULL && i->irq == irq) {
			mtx_unlock(&intr_table_lock);
			return (i);
		}
	}

	i = malloc(sizeof(*i), M_INTR, M_NOWAIT);
	if (i == NULL) {
		mtx_unlock(&intr_table_lock);
		return (NULL);
	}

	i->event = NULL;
	i->cntp = NULL;
	i->trig = INTR_TRIGGER_CONFORM;
	i->pol = INTR_POLARITY_CONFORM;
	i->irq = irq;
	i->pic = NULL;
	i->vector = -1;

#ifdef SMP
	i->cpu = all_cpus;
#else
	i->cpu = 1;
#endif

	for (vector = 0; vector < INTR_VECTORS && vector <= nvectors;
	    vector++) {
		iscan = powerpc_intrs[vector];
		if (iscan != NULL && iscan->irq == irq)
			break;
		if (iscan == NULL && i->vector == -1)
			i->vector = vector;
		iscan = NULL;
	}

	if (iscan == NULL && i->vector != -1) {
		powerpc_intrs[i->vector] = i;
		i->cntindex = atomic_fetchadd_int(&intrcnt_index, 1);
		i->cntp = &intrcnt[i->cntindex];
		sprintf(intrname, "irq%u:", i->irq);
		intrcnt_setname(intrname, i->cntindex);
		nvectors++;
	}
	mtx_unlock(&intr_table_lock);

	if (iscan != NULL || i->vector == -1) {
		free(i, M_INTR);
		i = iscan;
	}

	return (i);
}

static int
powerpc_map_irq(struct powerpc_intr *i)
{
	struct pic *p;
	u_int cnt;
	int idx;

	for (idx = 0; idx < npics; idx++) {
		p = &piclist[idx];
		cnt = p->irqs + p->ipis;
		if (i->irq >= p->base && i->irq < p->base + cnt)
			break;
	}
	if (idx == npics)
		return (EINVAL);

	i->intline = i->irq - p->base;
	i->pic = p->dev;

	/* Try a best guess if that failed */
	if (i->pic == NULL)
		i->pic = root_pic;

	return (0);
}

static void
powerpc_intr_eoi(void *arg)
{
	struct powerpc_intr *i = arg;

	PIC_EOI(i->pic, i->intline);
}

static void
powerpc_intr_pre_ithread(void *arg)
{
	struct powerpc_intr *i = arg;

	PIC_MASK(i->pic, i->intline);
	PIC_EOI(i->pic, i->intline);
}

static void
powerpc_intr_post_ithread(void *arg)
{
	struct powerpc_intr *i = arg;

	PIC_UNMASK(i->pic, i->intline);
}

static int
powerpc_assign_intr_cpu(void *arg, u_char cpu)
{
#ifdef SMP
	struct powerpc_intr *i = arg;

	if (cpu == NOCPU)
		i->cpu = all_cpus;
	else
		i->cpu = 1 << cpu;

	if (!cold && i->pic != NULL && i->pic == root_pic)
		PIC_BIND(i->pic, i->intline, i->cpu);

	return (0);
#else
	return (EOPNOTSUPP);
#endif
}

void
powerpc_register_pic(device_t dev, uint32_t node, u_int irqs, u_int ipis,
    u_int atpic)
{
	struct pic *p;
	u_int irq;
	int idx;

	mtx_lock(&intr_table_lock);

	/* XXX see powerpc_get_irq(). */
	for (idx = 0; idx < npics; idx++) {
		p = &piclist[idx];
		if (p->node != node)
			continue;
		if (node != 0 || p->dev == dev)
			break;
	}
	p = &piclist[idx];

	p->dev = dev;
	p->node = node;
	p->irqs = irqs;
	p->ipis = ipis;
	if (idx == npics) {
#ifdef DEV_ISA
		p->base = (atpic) ? 0 : nirqs;
#else
		p->base = nirqs;
#endif
		irq = p->base + irqs + ipis;
		nirqs = MAX(nirqs, irq);
		npics++;
	}

	mtx_unlock(&intr_table_lock);
}

u_int
powerpc_get_irq(uint32_t node, u_int pin)
{
	int idx;

	if (node == 0)
		return (pin);

	mtx_lock(&intr_table_lock);
	for (idx = 0; idx < npics; idx++) {
		if (piclist[idx].node == node) {
			mtx_unlock(&intr_table_lock);
			return (piclist[idx].base + pin);
		}
	}

	/*
	 * XXX we should never encounter an unregistered PIC, but that
	 * can only be done when we properly support bus enumeration
	 * using multiple passes. Until then, fake an entry and give it
	 * some adhoc maximum number of IRQs and IPIs.
	 */
	piclist[idx].dev = NULL;
	piclist[idx].node = node;
	piclist[idx].irqs = 124;
	piclist[idx].ipis = 4;
	piclist[idx].base = nirqs;
	nirqs += 128;
	npics++;

	mtx_unlock(&intr_table_lock);

	return (piclist[idx].base + pin);
}

int
powerpc_enable_intr(void)
{
	struct powerpc_intr *i;
	int error, vector;
#ifdef SMP
	int n;
#endif

	if (npics == 0)
		panic("no PIC detected\n");

	if (root_pic == NULL)
		root_pic = piclist[0].dev;

#ifdef SMP
	/* Install an IPI handler. */
	for (n = 0; n < npics; n++) {
		if (piclist[n].dev != root_pic)
			continue;

		KASSERT(piclist[n].ipis != 0, ("%s", __func__));
		error = powerpc_setup_intr("IPI",
		    INTR_VEC(piclist[n].node, piclist[n].irqs),
		    powerpc_ipi_handler, NULL, NULL,
		    INTR_TYPE_MISC | INTR_EXCL, &ipi_cookie);
		if (error) {
			printf("unable to setup IPI handler\n");
			return (error);
		}
	}
#endif

	for (vector = 0; vector < nvectors; vector++) {
		i = powerpc_intrs[vector];
		if (i == NULL)
			continue;

		error = powerpc_map_irq(i);
		if (error)
			continue;

		if (i->trig != INTR_TRIGGER_CONFORM ||
		    i->pol != INTR_POLARITY_CONFORM)
			PIC_CONFIG(i->pic, i->intline, i->trig, i->pol);

		if (i->event != NULL)
			PIC_ENABLE(i->pic, i->intline, vector);
	}

	return (0);
}

int
powerpc_setup_intr(const char *name, u_int irq, driver_filter_t filter,
    driver_intr_t handler, void *arg, enum intr_type flags, void **cookiep)
{
	struct powerpc_intr *i;
	int error, enable = 0;

	i = intr_lookup(irq);
	if (i == NULL)
		return (ENOMEM);

	if (i->event == NULL) {
		error = intr_event_create(&i->event, (void *)i, 0, irq,
		    powerpc_intr_pre_ithread, powerpc_intr_post_ithread,
		    powerpc_intr_eoi, powerpc_assign_intr_cpu, "irq%u:", irq);
		if (error)
			return (error);

		enable = 1;
	}

	error = intr_event_add_handler(i->event, name, filter, handler, arg,
	    intr_priority(flags), flags, cookiep);

	mtx_lock(&intr_table_lock);
	intrcnt_setname(i->event->ie_fullname, i->cntindex);
	mtx_unlock(&intr_table_lock);

	if (!cold) {
		error = powerpc_map_irq(i);

		if (!error && (i->trig != INTR_TRIGGER_CONFORM ||
		    i->pol != INTR_POLARITY_CONFORM))
			PIC_CONFIG(i->pic, i->intline, i->trig, i->pol);

		if (!error && i->pic == root_pic)
			PIC_BIND(i->pic, i->intline, i->cpu);

		if (!error && enable)
			PIC_ENABLE(i->pic, i->intline, i->vector);
	}
	return (error);
}

int
powerpc_teardown_intr(void *cookie)
{

	return (intr_event_remove_handler(cookie));
}

#ifdef SMP
int
powerpc_bind_intr(u_int irq, u_char cpu)
{
	struct powerpc_intr *i;

	i = intr_lookup(irq);
	if (i == NULL)
		return (ENOMEM);

	return (intr_event_bind(i->event, cpu));
}
#endif

int
powerpc_config_intr(int irq, enum intr_trigger trig, enum intr_polarity pol)
{
	struct powerpc_intr *i;

	i = intr_lookup(irq);
	if (i == NULL)
		return (ENOMEM);

	i->trig = trig;
	i->pol = pol;

	if (!cold && i->pic != NULL)
		PIC_CONFIG(i->pic, i->intline, trig, pol);

	return (0);
}

void
powerpc_dispatch_intr(u_int vector, struct trapframe *tf)
{
	struct powerpc_intr *i;
	struct intr_event *ie;

	i = powerpc_intrs[vector];
	if (i == NULL)
		goto stray;

	(*i->cntp)++;

	ie = i->event;
	KASSERT(ie != NULL, ("%s: interrupt without an event", __func__));

	if (intr_event_handle(ie, tf) != 0) {
		goto stray;
	}
	return;

stray:
	stray_count++;
	if (stray_count <= MAX_STRAY_LOG) {
		printf("stray irq %d\n", i ? i->irq : -1);
		if (stray_count >= MAX_STRAY_LOG) {
			printf("got %d stray interrupts, not logging anymore\n",
			    MAX_STRAY_LOG);
		}
	}
	if (i != NULL)
		PIC_MASK(i->pic, i->intline);
}
