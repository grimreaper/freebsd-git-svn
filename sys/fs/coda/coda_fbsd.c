/*
 *             Coda: an Experimental Distributed File System
 *                              Release 3.1
 * 
 *           Copyright (c) 1987-1998 Carnegie Mellon University
 *                          All Rights Reserved
 * 
 * Permission  to  use, copy, modify and distribute this software and its
 * documentation is hereby granted,  provided  that  both  the  copyright
 * notice  and  this  permission  notice  appear  in  all  copies  of the
 * software, derivative works or  modified  versions,  and  any  portions
 * thereof, and that both notices appear in supporting documentation, and
 * that credit is given to Carnegie Mellon University  in  all  documents
 * and publicity pertaining to direct or indirect use of this code or its
 * derivatives.
 * 
 * CODA IS AN EXPERIMENTAL SOFTWARE SYSTEM AND IS  KNOWN  TO  HAVE  BUGS,
 * SOME  OF  WHICH MAY HAVE SERIOUS CONSEQUENCES.  CARNEGIE MELLON ALLOWS
 * FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.   CARNEGIE  MELLON
 * DISCLAIMS  ANY  LIABILITY  OF  ANY  KIND  FOR  ANY  DAMAGES WHATSOEVER
 * RESULTING DIRECTLY OR INDIRECTLY FROM THE USE OF THIS SOFTWARE  OR  OF
 * ANY DERIVATIVE WORK.
 * 
 * Carnegie  Mellon  encourages  users  of  this  software  to return any
 * improvements or extensions that  they  make,  and  to  grant  Carnegie
 * Mellon the rights to redistribute these changes without encumbrance.
 * 
 * 	@(#) src/sys/coda/coda_fbsd.cr,v 1.1.1.1 1998/08/29 21:14:52 rvb Exp $
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/ucred.h>
#include <sys/vnode.h>

#include <vm/vm.h>
#include <vm/vnode_pager.h>

#include <coda/coda.h>
#include <coda/cnode.h>
#include <coda/coda_vnops.h>
#include <coda/coda_psdev.h>

static struct cdevsw codadevsw = {
	.d_version =	D_VERSION,
	.d_flags =	D_NEEDGIANT,
	.d_open =	vc_nb_open,
	.d_close =	vc_nb_close,
	.d_read =	vc_nb_read,
	.d_write =	vc_nb_write,
	.d_ioctl =	vc_nb_ioctl,
	.d_poll =	vc_nb_poll,
	.d_name =	"Coda",
};

static eventhandler_tag clonetag;

static LIST_HEAD(, coda_mntinfo) coda_mnttbl;

int     vcdebug = 1;
#define VCDEBUG if (vcdebug) printf

/* for DEVFS, using bpf & tun drivers as examples*/
static void coda_fbsd_clone(void *arg, char *name, int namelen,
    struct cdev **dev);

static int
codadev_modevent(module_t mod, int type, void *data)
{
	struct coda_mntinfo	*mnt;

	switch (type) {
	case MOD_LOAD:
		LIST_INIT(&coda_mnttbl);
		clonetag = EVENTHANDLER_REGISTER(dev_clone, coda_fbsd_clone,
		    0, 1000);
		break;
	case MOD_UNLOAD:
		EVENTHANDLER_DEREGISTER(dev_clone, clonetag);
		while ((mnt = LIST_FIRST(&coda_mnttbl)) != NULL) {
			LIST_REMOVE(mnt, mi_list);
			destroy_dev(mnt->dev);
			free(mnt, M_CODA);
		}
		break;

	default:
		return (EOPNOTSUPP);
	}
	return 0;
}
static moduledata_t codadev_mod = {
	"codadev",
	codadev_modevent,
	NULL
};
DECLARE_MODULE(codadev, codadev_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);

int
coda_fbsd_getpages(v)
	void *v;
{
    struct vop_getpages_args *ap = v;

#if	1
	return vop_stdgetpages(ap);
#else
  {
    struct vnode *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    struct vnode *cfvp = cp->c_ovp;
    int opened_internally = 0;
    struct ucred *cred = (struct ucred *) 0;
    struct proc *p = curproc;
    int error = 0;
	
    if (IS_CTL_VP(vp)) {
	return(EINVAL);
    }

    /* Redirect the request to UFS. */

    if (cfvp == NULL) {
	opened_internally = 1;

	error = VOP_OPEN(vp, FREAD,  cred, p, -1);
printf("coda_getp: Internally Opening %p\n", vp);

	if (error) {
	    printf("coda_getpage: VOP_OPEN on container failed %d\n", error);
		return (error);
	}
	if (vp->v_type == VREG) {
	    error = vfs_object_create(vp, p, cred);
	    if (error != 0) {
		printf("coda_getpage: vfs_object_create() returns %d\n", error);
		vput(vp);
		return(error);
	    }
	}

	cfvp = cp->c_ovp;
    } else {
printf("coda_getp: has container %p\n", cfvp);
    }

printf("coda_fbsd_getpages: using container ");
/*
    error = vnode_pager_generic_getpages(cfvp, ap->a_m, ap->a_count,
	ap->a_reqpage);
*/
    error = VOP_GETPAGES(cfvp, ap->a_m, ap->a_count,
	ap->a_reqpage, ap->a_offset);
printf("error = %d\n", error);

    /* Do an internal close if necessary. */
    if (opened_internally) {
	(void)VOP_CLOSE(vp, FREAD, cred, p);
    }

    return(error);
  }
#endif
}

static void coda_fbsd_clone(arg, name, namelen, dev)
    void *arg;
    char *name;
    int namelen;
    struct cdev **dev;
{
    int u;
    struct coda_mntinfo *mnt;

    if (*dev != NULL)
	return;
    if (dev_stdclone(name,NULL,"cfs",&u) != 1)
	return;

    *dev = make_dev(&codadevsw,unit2minor(u),UID_ROOT,GID_WHEEL,0600,"cfs%d",u);
    mnt = malloc(sizeof(struct coda_mntinfo), M_CODA, M_WAITOK|M_ZERO);
    LIST_INSERT_HEAD(&coda_mnttbl, mnt, mi_list);
}

struct coda_mntinfo *
dev2coda_mntinfo(struct cdev *dev)
{
	struct coda_mntinfo	*mnt;

	LIST_FOREACH(mnt, &coda_mnttbl, mi_list) {
		if (mnt->dev == dev)
			break;
	}

	return mnt;
}
