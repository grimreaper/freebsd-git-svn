/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)vnode.h	8.7 (Berkeley) 2/4/94
 * $FreeBSD$
 */

#ifndef _SYS_VNODE_H_
#define	_SYS_VNODE_H_

/*
 * XXX - compatability until lockmgr() goes away or all the #includes are
 * updated.
 */
#include <sys/lockmgr.h>

#include <sys/queue.h>
#include <sys/_lock.h>
#include <sys/_mutex.h>
#include <sys/selinfo.h>
#include <sys/uio.h>
#include <sys/acl.h>
#include <vm/uma.h>

/*
 * The vnode is the focus of all file activity in UNIX.  There is a
 * unique vnode allocated for each active file, each current directory,
 * each mounted-on file, text file, and the root.
 */

/*
 * Vnode types.  VNON means no type.
 */
enum vtype	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };

/*
 * Vnode tag types.
 * These are for the benefit of external programs only (e.g., pstat)
 * and should NEVER be inspected by the kernel.
 */
enum vtagtype	{
	VT_NON, VT_UFS, VT_NFS, VT_UNUSED, VT_PC, VT_LFS, VT_LOFS, VT_FDESC,
	VT_PORTAL, VT_NULL, VT_UMAP, VT_KERNFS, VT_PROCFS, VT_AFS, VT_ISOFS,
	VT_UNION, VT_MSDOSFS, VT_DEVFS, VT_TFS, VT_VFS, VT_CODA, VT_NTFS,
	VT_HPFS, VT_NWFS, VT_PSEUDOFS, VT_SMBFS, VT_UDF
};

/*
 * Each underlying filesystem allocates its own private area and hangs
 * it from v_data.  If non-null, this area is freed in getnewvnode().
 */
TAILQ_HEAD(buflists, buf);

typedef	int	vop_t(void *);
struct namecache;

struct vpollinfo {
	struct	mtx vpi_lock;		/* lock to protect below */
	struct	selinfo vpi_selinfo;	/* identity of poller(s) */
	short	vpi_events;		/* what they are looking for */
	short	vpi_revents;		/* what has happened */
};

/*
 * Reading or writing any of these items requires holding the appropriate lock.
 * v_freelist is locked by the global vnode_free_list mutex.
 * v_mntvnodes is locked by the global mntvnodes mutex.
 * v_flag, v_usecount, v_holdcount and v_writecount are
 *    locked by the v_interlock mutex.
 * v_pollinfo is locked by the lock contained inside it.
 */
struct vnode {
	u_long	v_flag;				/* vnode flags (see below) */
	int	v_usecount;			/* reference count of users */
	int	v_writecount;			/* reference count of writers */
	int	v_holdcnt;			/* page & buffer references */
	u_long	v_id;				/* capability identifier */
	struct	mount *v_mount;			/* ptr to vfs we are in */
	vop_t	**v_op;				/* vnode operations vector */
	TAILQ_ENTRY(vnode) v_freelist;		/* vnode freelist */
	TAILQ_ENTRY(vnode) v_nmntvnodes;	/* vnodes for mount point */
	struct	buflists v_cleanblkhd;		/* clean blocklist head */
	struct	buflists v_dirtyblkhd;		/* dirty blocklist head */
	LIST_ENTRY(vnode) v_synclist;		/* vnodes with dirty buffers */
	long	v_numoutput;			/* num of writes in progress */
	enum	vtype v_type;			/* vnode type */
	union {
		struct mount	*vu_mountedhere;/* ptr to mounted vfs (VDIR) */
		struct socket	*vu_socket;	/* unix ipc (VSOCK) */
		struct {
			struct specinfo	*vu_specinfo; /* device (VCHR, VBLK) */
			SLIST_ENTRY(vnode) vu_specnext;
		} vu_spec;
		struct fifoinfo	*vu_fifoinfo;	/* fifo (VFIFO) */
	} v_un;
	daddr_t	v_lastw;			/* last write (write cluster) */
	daddr_t	v_cstart;			/* start block of cluster */
	daddr_t	v_lasta;			/* last allocation (cluster) */
	int	v_clen;				/* length of current cluster */
	struct vm_object *v_object;		/* Place to store VM object */
	struct	mtx v_interlock;		/* lock on usecount and flag */
	struct	lock v_lock;			/* used if fs don't have one */
	struct	lock *v_vnlock;			/* pointer to vnode lock */
	enum	vtagtype v_tag;			/* type of underlying data */
	void	*v_data;			/* private data for fs */
	LIST_HEAD(, namecache) v_cache_src;	/* Cache entries from us */
	TAILQ_HEAD(, namecache) v_cache_dst;	/* Cache entries to us */
	struct	vnode *v_dd;			/* .. vnode */
	u_long	v_ddid;				/* .. capability identifier */
	struct vpollinfo *v_pollinfo;
	struct thread *v_vxproc;		/* thread owning VXLOCK */
#ifdef	DEBUG_LOCKS
	const char *filename;			/* Source file doing locking */
	int line;				/* Line number doing locking */
#endif
};
#define	v_mountedhere	v_un.vu_mountedhere
#define	v_socket	v_un.vu_socket
#define	v_rdev		v_un.vu_spec.vu_specinfo
#define	v_specnext	v_un.vu_spec.vu_specnext
#define	v_fifoinfo	v_un.vu_fifoinfo

#define	VN_POLLEVENT(vp, events)				\
	do {							\
		if ((vp)->v_pollinfo != NULL && 		\
		    (vp)->v_pollinfo->vpi_events & (events))	\
			vn_pollevent((vp), (events));		\
	} while (0)

#define VN_KNOTE(vp, b)						\
	do {							\
		if ((vp)->v_pollinfo != NULL)			\
			KNOTE(&vp->v_pollinfo->vpi_selinfo.si_note, (b)); \
	} while (0)

/*
 * Vnode flags.
 */
#define	VROOT		0x00001	/* root of its filesystem */
#define	VTEXT		0x00002	/* vnode is a pure text prototype */
#define	VSYSTEM		0x00004	/* vnode being used by kernel */
#define	VISTTY		0x00008	/* vnode represents a tty */
#define	VXLOCK		0x00100	/* vnode is locked to change underlying type */
#define	VXWANT		0x00200	/* thread is waiting for vnode */
#define	VBWAIT		0x00400	/* waiting for output to complete */
#define	VNOSYNC		0x01000	/* unlinked, stop syncing */
/* open for business    0x01000 */
#define	VOBJBUF		0x02000	/* Allocate buffers in VM object */
#define	VCOPYONWRITE    0x04000 /* vnode is doing copy-on-write */
#define	VAGE		0x08000	/* Insert vnode at head of free list */
#define	VOLOCK		0x10000	/* vnode is locked waiting for an object */
#define	VOWANT		0x20000	/* a thread is waiting for VOLOCK */
#define	VDOOMED		0x40000	/* This vnode is being recycled */
#define	VFREE		0x80000	/* This vnode is on the freelist */
/* open for business	0x100000 */
#define	VONWORKLST	0x200000 /* On syncer work-list */
#define	VMOUNT		0x400000 /* Mount in progress */
#define	VOBJDIRTY	0x800000 /* object might be dirty */

/*
 * Vnode attributes.  A field value of VNOVAL represents a field whose value
 * is unavailable (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	short		va_nlink;	/* number of references to file */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
	udev_t		va_fsid;	/* filesystem id */
	long		va_fileid;	/* file id */
	u_quad_t	va_size;	/* file size in bytes */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timespec	va_atime;	/* time of last access */
	struct timespec	va_mtime;	/* time of last modification */
	struct timespec	va_ctime;	/* time file changed */
	u_long		va_gen;		/* generation number of file */
	u_long		va_flags;	/* flags defined for file */
	udev_t		va_rdev;	/* device the special file represents */
	u_quad_t	va_bytes;	/* bytes of disk space held by file */
	u_quad_t	va_filerev;	/* file modification number */
	u_int		va_vaflags;	/* operations flags, see below */
	long		va_spare;	/* remain quad aligned */
};

/*
 * Flags for va_vaflags.
 */
#define	VA_UTIMES_NULL	0x01		/* utimes argument was NULL */
#define	VA_EXCLUSIVE	0x02		/* exclusive create request */

/*
 * Flags for ioflag. (high 16 bits used to ask for read-ahead and
 * help with write clustering)
 */
#define	IO_UNIT		0x01		/* do I/O as atomic unit */
#define	IO_APPEND	0x02		/* append write to end */
#define	IO_SYNC		0x04		/* do I/O synchronously */
#define	IO_NODELOCKED	0x08		/* underlying node already locked */
#define	IO_NDELAY	0x10		/* FNDELAY flag set in file table */
#define	IO_VMIO		0x20		/* data already in VMIO space */
#define	IO_INVAL	0x40		/* invalidate after I/O */
#define	IO_ASYNC	0x80		/* bawrite rather then bdwrite */
#define IO_DIRECT	0x100		/* attempt to bypass buffer cache */
#define IO_NOWDRAIN	0x200		/* do not block on wdrain */

/*
 *  Modes.  Some values same as Ixxx entries from inode.h for now.
 */
#define	VADMIN	010000		/* permission to administer vnode */
#define	VSUID	004000		/* set user id on execution */
#define	VSGID	002000		/* set group id on execution */
#define	VSVTX	001000		/* save swapped text even after use */
#define	VREAD	000400		/* read, write, execute permissions */
#define	VWRITE	000200
#define	VEXEC	000100

/*
 * Token indicating no attribute value yet assigned.
 */
#define	VNOVAL	(-1)

/*
 * LK_TIMELOCK timeout for vnode locks (used mainly by the pageout daemon)
 */
#define VLKTIMEOUT	(hz / 20 + 1)

#ifdef _KERNEL

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_VNODE);
#endif

/*
 * Convert between vnode types and inode formats (since POSIX.1
 * defines mode word of stat structure in terms of inode formats).
 */
extern enum vtype	iftovt_tab[];
extern int		vttoif_tab[];
#define	IFTOVT(mode)	(iftovt_tab[((mode) & S_IFMT) >> 12])
#define	VTTOIF(indx)	(vttoif_tab[(int)(indx)])
#define	MAKEIMODE(indx, mode)	(int)(VTTOIF(indx) | (mode))

/*
 * Flags to various vnode functions.
 */
#define	SKIPSYSTEM	0x0001	/* vflush: skip vnodes marked VSYSTEM */
#define	FORCECLOSE	0x0002	/* vflush: force file closure */
#define	WRITECLOSE	0x0004	/* vflush: only close writable files */
#define	DOCLOSE		0x0008	/* vclean: close active files */
#define	V_SAVE		0x0001	/* vinvalbuf: sync file first */
#define	REVOKEALL	0x0001	/* vop_revoke: revoke all aliases */
#define	V_WAIT		0x0001	/* vn_start_write: sleep for suspend */
#define	V_NOWAIT	0x0002	/* vn_start_write: don't sleep for suspend */
#define	V_XSLEEP	0x0004	/* vn_start_write: just return after sleep */

#define	VREF(vp)	vref(vp)


#ifdef DIAGNOSTIC
#define	VATTR_NULL(vap)	vattr_null(vap)
#else
#define	VATTR_NULL(vap)	(*(vap) = va_null)	/* initialize a vattr */
#endif /* DIAGNOSTIC */

#define	NULLVP	((struct vnode *)NULL)

#define	VNODEOP_SET(f) \
	C_SYSINIT(f##init, SI_SUB_VFS, SI_ORDER_SECOND, vfs_add_vnodeops, &f); \
	C_SYSUNINIT(f##uninit, SI_SUB_VFS, SI_ORDER_SECOND, vfs_rm_vnodeops, &f);

/*
 * Global vnode data.
 */
extern	struct vnode *rootvnode;	/* root (i.e. "/") vnode */
extern	int desiredvnodes;		/* number of vnodes desired */
extern	uma_zone_t namei_zone;
extern	int prtactive;			/* nonzero to call vprint() */
extern	struct vattr va_null;		/* predefined null vattr structure */
extern	int vfs_ioopt;

/*
 * Macro/function to check for client cache inconsistency w.r.t. leasing.
 */
#define	LEASE_READ	0x1		/* Check lease for readers */
#define	LEASE_WRITE	0x2		/* Check lease for modifiers */


extern void	(*lease_updatetime)(int deltat);

#define	VSHOULDFREE(vp)	\
	(!((vp)->v_flag & (VFREE|VDOOMED)) && \
	 !(vp)->v_holdcnt && !(vp)->v_usecount && \
	 (!(vp)->v_object || \
	  !((vp)->v_object->ref_count || (vp)->v_object->resident_page_count)))

#define VMIGHTFREE(vp) \
	(!((vp)->v_flag & (VFREE|VDOOMED|VXLOCK)) &&	\
	 LIST_EMPTY(&(vp)->v_cache_src) && !(vp)->v_usecount)

#define	VSHOULDBUSY(vp)	\
	(((vp)->v_flag & VFREE) && \
	 ((vp)->v_holdcnt || (vp)->v_usecount))

#define	VI_LOCK(vp)	mtx_lock(&(vp)->v_interlock)
#define	VI_TRYLOCK(vp)	mtx_trylock(&(vp)->v_interlock)
#define	VI_UNLOCK(vp)	mtx_unlock(&(vp)->v_interlock)

#endif /* _KERNEL */


/*
 * Mods for extensibility.
 */

/*
 * Flags for vdesc_flags:
 */
#define	VDESC_MAX_VPS		16
/* Low order 16 flag bits are reserved for willrele flags for vp arguments. */
#define	VDESC_VP0_WILLRELE	0x0001
#define	VDESC_VP1_WILLRELE	0x0002
#define	VDESC_VP2_WILLRELE	0x0004
#define	VDESC_VP3_WILLRELE	0x0008
#define	VDESC_NOMAP_VPP		0x0100
#define	VDESC_VPP_WILLRELE	0x0200

/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define VDESC_NO_OFFSET -1

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int	 vdesc_offset;		/* offset in vector,first for speed */
	char	*vdesc_name;		/* a readable name for debugging */
	int	 vdesc_flags;		/* VDESC_* flags */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	int	*vdesc_vp_offsets;	/* list ended by VDESC_NO_OFFSET */
	int	vdesc_vpp_offset;	/* return vpp location */
	int	vdesc_cred_offset;	/* cred location, if any */
	int	vdesc_thread_offset;	/* thread location, if any */
	int	vdesc_componentname_offset; /* if any */
	/*
	 * Finally, we've got a list of private data (about each operation)
	 * for each transport layer.  (Support to manage this list is not
	 * yet part of BSD.)
	 */
	caddr_t	*vdesc_transports;
};

#ifdef _KERNEL
/*
 * A list of all the operation descs.
 */
extern struct vnodeop_desc *vnodeop_descs[];

/*
 * Interlock for scanning list of vnodes attached to a mountpoint
 */
extern struct mtx mntvnode_mtx;

/*
 * This macro is very helpful in defining those offsets in the vdesc struct.
 *
 * This is stolen from X11R4.  I ignored all the fancy stuff for
 * Crays, so if you decide to port this to such a serious machine,
 * you might want to consult Intrinsic.h's XtOffset{,Of,To}.
 */
#define	VOPARG_OFFSET(p_type,field) \
	((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#define	VOPARG_OFFSETOF(s_type,field) \
	VOPARG_OFFSET(s_type*,field)
#define	VOPARG_OFFSETTO(S_TYPE,S_OFFSET,STRUCT_P) \
	((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))


/*
 * This structure is used to configure the new vnodeops vector.
 */
struct vnodeopv_entry_desc {
	struct vnodeop_desc *opve_op;   /* which operation this is */
	vop_t *opve_impl;		/* code implementing this operation */
};
struct vnodeopv_desc {
			/* ptr to the ptr to the vector where op should go */
	vop_t ***opv_desc_vector_p;
	struct vnodeopv_entry_desc *opv_desc_ops;   /* null terminated list */
};

/*
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeop_desc *a_desc;
	/* other random data follows, presumably */
};


#ifdef DEBUG_VFS_LOCKS
/*
 * Macros to aid in tracing VFS locking problems.  Not totally
 * reliable since if the thread sleeps between changing the lock
 * state and checking it with the assert, some other thread could
 * change the state.  They are good enough for debugging a single
 * filesystem using a single-threaded test.  I find that 'cvs co src'
 * is a pretty good test.
 */

/*
 * [dfr] Kludge until I get around to fixing all the vfs locking.
 */
#define IS_LOCKING_VFS(vp)	((vp)->v_tag == VT_UFS			\
				 || (vp)->v_tag == VT_NFS		\
				 || (vp)->v_tag == VT_LFS		\
				 || (vp)->v_tag == VT_ISOFS		\
				 || (vp)->v_tag == VT_MSDOSFS		\
				 || (vp)->v_tag == VT_DEVFS		\
				 || (vp)->v_tag == VT_UDF)

#define ASSERT_VOP_LOCKED(vp, str)					\
do {									\
	struct vnode *_vp = (vp);					\
									\
	if (_vp && IS_LOCKING_VFS(_vp) && !VOP_ISLOCKED(_vp, NULL))	\
		panic("%s: %p is not locked but should be", str, _vp);	\
} while (0)

#define ASSERT_VOP_UNLOCKED(vp, str)					\
do {									\
	struct vnode *_vp = (vp);					\
	int lockstate;							\
									\
	if (_vp && IS_LOCKING_VFS(_vp)) {				\
		lockstate = VOP_ISLOCKED(_vp, curthread);		\
		if (lockstate == LK_EXCLUSIVE)				\
			panic("%s: %p is locked but should not be",	\
			    str, _vp);					\
	}								\
} while (0)

#define ASSERT_VOP_ELOCKED(vp, str)					\
do {									\
	struct vnode *_vp = (vp);					\
									\
	if (_vp && IS_LOCKING_VFS(_vp) &&				\
	    VOP_ISLOCKED(_vp, curthread) != LK_EXCLUSIVE)			\
		panic("%s: %p is not exclusive locked but should be",	\
		    str, _vp);						\
} while (0)

#define ASSERT_VOP_ELOCKED_OTHER(vp, str)				\
do {									\
	struct vnode *_vp = (vp);					\
									\
	if (_vp && IS_LOCKING_VFS(_vp) &&				\
	    VOP_ISLOCKED(_vp, curthread) != LK_EXCLOTHER)			\
		panic("%s: %p is not exclusive locked by another thread",	\
		    str, _vp);						\
} while (0)

#define ASSERT_VOP_SLOCKED(vp, str)					\
do {									\
	struct vnode *_vp = (vp);					\
									\
	if (_vp && IS_LOCKING_VFS(_vp) &&				\
	    VOP_ISLOCKED(_vp, NULL) != LK_SHARED)			\
		panic("%s: %p is not locked shared but should be",	\
		    str, _vp);						\
} while (0)

#else

#define ASSERT_VOP_LOCKED(vp, str)
#define ASSERT_VOP_UNLOCKED(vp, str)

#endif

/*
 * VOCALL calls an op given an ops vector.  We break it out because BSD's
 * vclean changes the ops vector and then wants to call ops with the old
 * vector.
 */
#define VOCALL(OPSV,OFF,AP) (( *((OPSV)[(OFF)])) (AP))

/*
 * This call works for vnodes in the kernel.
 */
#define VCALL(VP,OFF,AP) VOCALL((VP)->v_op,(OFF),(AP))
#define VDESC(OP) (& __CONCAT(OP,_desc))
#define VOFFSET(OP) (VDESC(OP)->vdesc_offset)

/*
 * VMIO support inline
 */

extern int vmiodirenable;

static __inline int
vn_canvmio(struct vnode *vp)
{
      if (vp && (vp->v_type == VREG || (vmiodirenable && vp->v_type == VDIR)))
		return(TRUE);
	return(FALSE);
}

/*
 * Finally, include the default set of vnode operations.
 */
#include "vnode_if.h"

/*
 * Public vnode manipulation functions.
 */
struct componentname;
struct file;
struct mount;
struct nameidata;
struct ostat;
struct thread;
struct proc;
struct stat;
struct nstat;
struct ucred;
struct uio;
struct vattr;
struct vnode;

extern int	(*lease_check_hook)(struct vop_lease_args *);

struct	vnode *addaliasu(struct vnode *vp, udev_t nvp_rdev);
int	bdevvp(dev_t dev, struct vnode **vpp);
/* cache_* may belong in namei.h. */
void	cache_enter(struct vnode *dvp, struct vnode *vp,
	    struct componentname *cnp);
int	cache_lookup(struct vnode *dvp, struct vnode **vpp,
	    struct componentname *cnp);
void	cache_purge(struct vnode *vp);
void	cache_purgevfs(struct mount *mp);
int	cache_leaf_test(struct vnode *vp);
void	cvtstat(struct stat *st, struct ostat *ost);
void	cvtnstat(struct stat *sb, struct nstat *nsb);
int	getnewvnode(enum vtagtype tag,
	    struct mount *mp, vop_t **vops, struct vnode **vpp);
int	lease_check(struct vop_lease_args *ap);
int	spec_vnoperate(struct vop_generic_args *);
int	speedup_syncer(void);
#define textvp_fullpath(p, rb, rfb) \
	vn_fullpath(FIRST_THREAD_IN_PROC(p), (p)->p_textvp, rb, rfb)
int	vn_fullpath(struct thread *td, struct vnode *vn,
	    char **retbuf, char **freebuf);
int	vaccess(enum vtype type, mode_t file_mode, uid_t uid, gid_t gid,
	    mode_t acc_mode, struct ucred *cred, int *privused);
int	vaccess_acl_posix1e(enum vtype type, uid_t file_uid,
	    gid_t file_gid, struct acl *acl, mode_t acc_mode,
	    struct ucred *cred, int *privused);
void	vattr_null(struct vattr *vap);
int	vcount(struct vnode *vp);
void	vdrop(struct vnode *);
int	vfinddev(dev_t dev, enum vtype type, struct vnode **vpp);
void	vfs_add_vnodeops(const void *);
void	vfs_rm_vnodeops(const void *);
int	vflush(struct mount *mp, int rootrefs, int flags);
int	vget(struct vnode *vp, int lockflag, struct thread *td);
void	vgone(struct vnode *vp);
void	vgonel(struct vnode *vp, struct thread *td);
void	vhold(struct vnode *);
int	vinvalbuf(struct vnode *vp, int save, struct ucred *cred,
	    struct thread *td, int slpflag, int slptimeo);
int	vtruncbuf(struct vnode *vp, struct ucred *cred, struct thread *td,
	    off_t length, int blksize);
void	vprint(char *label, struct vnode *vp);
int	vrecycle(struct vnode *vp, struct mtx *inter_lkp,
	    struct thread *td);
int	vn_close(struct vnode *vp,
	    int flags, struct ucred *cred, struct thread *td);
void	vn_finished_write(struct mount *mp);
int	vn_isdisk(struct vnode *vp, int *errp);
int	vn_lock(struct vnode *vp, int flags, struct thread *td);
#ifdef	DEBUG_LOCKS
int	debug_vn_lock(struct vnode *vp, int flags, struct thread *p,
	    const char *filename, int line);
#define vn_lock(vp,flags,p) debug_vn_lock(vp,flags,p,__FILE__,__LINE__)
#endif
int	vn_mkdir(char *path, int mode, enum uio_seg segflg, struct thread *td);
int	vn_open(struct nameidata *ndp, int *flagp, int cmode);
int	vn_open_cred(struct nameidata *ndp, int *flagp, int cmode,
	    struct ucred *cred);
void	vn_pollevent(struct vnode *vp, int events);
void	vn_pollgone(struct vnode *vp);
int	vn_pollrecord(struct vnode *vp, struct thread *p, int events);
int	vn_rdwr(enum uio_rw rw, struct vnode *vp, caddr_t base,
	    int len, off_t offset, enum uio_seg segflg, int ioflg,
	    struct ucred *cred, int *aresid, struct thread *td);
int	vn_rdwr_inchunks(enum uio_rw rw, struct vnode *vp, caddr_t base,
	    int len, off_t offset, enum uio_seg segflg, int ioflg,
	    struct ucred *cred, int *aresid, struct thread *td);
int	vn_stat(struct vnode *vp, struct stat *sb, struct thread *td);
int	vn_start_write(struct vnode *vp, struct mount **mpp, int flags);
dev_t	vn_todev(struct vnode *vp);
int	vn_write_suspend_wait(struct vnode *vp, struct mount *mp,
	    int flags);
int	vn_writechk(struct vnode *vp);
int	vn_extattr_get(struct vnode *vp, int ioflg, int attrnamespace,
	    const char *attrname, int *buflen, char *buf, struct thread *td);
int	vn_extattr_set(struct vnode *vp, int ioflg, int attrnamespace,
	    const char *attrname, int buflen, char *buf, struct thread *td);
int	vn_extattr_rm(struct vnode *vp, int ioflg, int attrnamespace,
	    const char *attrname, struct thread *td);
int	vfs_cache_lookup(struct vop_lookup_args *ap);
int	vfs_object_create(struct vnode *vp, struct thread *td,
	    struct ucred *cred);
void	vfs_timestamp(struct timespec *);
void	vfs_write_resume(struct mount *mp);
void	vfs_write_suspend(struct mount *mp);
int	vop_stdbmap(struct vop_bmap_args *);
int	vop_stdgetwritemount(struct vop_getwritemount_args *);
int	vop_stdgetpages(struct vop_getpages_args *);
int	vop_stdinactive(struct vop_inactive_args *);
int	vop_stdislocked(struct vop_islocked_args *);
int	vop_stdlock(struct vop_lock_args *);
int	vop_stdputpages(struct vop_putpages_args *);
int	vop_stdunlock(struct vop_unlock_args *);
int	vop_noislocked(struct vop_islocked_args *);
int	vop_nolock(struct vop_lock_args *);
int	vop_nopoll(struct vop_poll_args *);
int	vop_nounlock(struct vop_unlock_args *);
int	vop_stdpathconf(struct vop_pathconf_args *);
int	vop_stdpoll(struct vop_poll_args *);
int	vop_revoke(struct vop_revoke_args *);
int	vop_sharedlock(struct vop_lock_args *);
int	vop_eopnotsupp(struct vop_generic_args *ap);
int	vop_ebadf(struct vop_generic_args *ap);
int	vop_einval(struct vop_generic_args *ap);
int	vop_enotty(struct vop_generic_args *ap);
int	vop_defaultop(struct vop_generic_args *ap);
int	vop_null(struct vop_generic_args *ap);
int	vop_panic(struct vop_generic_args *ap);
int	vop_stdcreatevobject(struct vop_createvobject_args *ap);
int	vop_stddestroyvobject(struct vop_destroyvobject_args *ap);
int	vop_stdgetvobject(struct vop_getvobject_args *ap);

void	vfree(struct vnode *);
void	vput(struct vnode *vp);
void	vrele(struct vnode *vp);
void	vref(struct vnode *vp);
void	vbusy(struct vnode *vp);
void 	v_addpollinfo(struct vnode *vp);

extern	vop_t **default_vnodeop_p;
extern	vop_t **spec_vnodeop_p;
extern	vop_t **dead_vnodeop_p;

#endif /* _KERNEL */

#endif /* !_SYS_VNODE_H_ */
