/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 *
 * $FreeBSD$
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#pragma	D depends_on library ip.d
#pragma	D depends_on library net.d
#pragma	D depends_on module nfs.d
#pragma D depends_on module nfssrv

#pragma D binding "1.5" translator
translator conninfo_t < rfs4_client_t *P > {
	ci_protocol = (P->cl_addr.ss_family == AF_INET) ? "ipv4" : "ipv6";

	ci_local = "<unknown>";

	ci_remote = (P->cl_addr.ss_family == AF_INET) ?
	    inet_ntoa((ipaddr_t *)
	    &((struct sockaddr_in *)&P->cl_addr)->sin_addr) :
	    inet_ntoa6(&((struct sockaddr_in6 *)&P->cl_addr)->sin6_addr);
};

#pragma D binding "1.5" translator
translator nfsv4cbinfo_t < rfs4_deleg_state_t *P > {
	nci_curpath = (P->finfo->vp == NULL) ? "<unknown>" :
	    P->finfo->vp->v_path;
};
