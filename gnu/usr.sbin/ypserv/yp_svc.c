/*
 * And thus spoke RPCGEN:
 *    Please do not edit this file.
 *    It was generated using rpcgen.
 *
 * And thus replied Lpd@NannyMUD:
 *    Who cares? :-) /Peter Eriksson <pen@signum.se>
 *
 *	$Id$
 */

#include "system.h"

#include "yp.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>
#include <paths.h>

extern int errno;
extern void Perror();

#ifdef __STDC__
#define SIG_PF void(*)(int)
#endif

static void
ypprog_2(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		domainname ypproc_domain_2_arg;
		domainname ypproc_domain_nonack_2_arg;
		ypreq_key ypproc_match_2_arg;
		ypreq_key ypproc_first_2_arg;
		ypreq_key ypproc_next_2_arg;
		ypreq_xfr ypproc_xfr_2_arg;
		ypreq_nokey ypproc_all_2_arg;
		ypreq_nokey ypproc_master_2_arg;
		ypreq_nokey ypproc_order_2_arg;
		domainname ypproc_maplist_2_arg;
	} argument;
	char *result;
	xdrproc_t __xdr_argument, __xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case YPPROC_NULL:
		__xdr_argument = (xdrproc_t) xdr_void;
		__xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) ypproc_null_2_svc;
		break;

	case YPPROC_DOMAIN:
		__xdr_argument = (xdrproc_t) __xdr_domainname;
		__xdr_result = (xdrproc_t) xdr_bool;
		local = (char *(*)(char *, struct svc_req *)) ypproc_domain_2_svc;
		break;

	case YPPROC_DOMAIN_NONACK:
		__xdr_argument = (xdrproc_t) __xdr_domainname;
		__xdr_result = (xdrproc_t) xdr_bool;
		local = (char *(*)(char *, struct svc_req *)) ypproc_domain_nonack_2_svc;
		break;

	case YPPROC_MATCH:
		__xdr_argument = (xdrproc_t) __xdr_ypreq_key;
		__xdr_result = (xdrproc_t) __xdr_ypresp_val;
		local = (char *(*)(char *, struct svc_req *)) ypproc_match_2_svc;
		break;

	case YPPROC_FIRST:
#if 0 /* Bug in Sun's yp.x RPC prototype file */
		__xdr_argument = (xdrproc_t) __xdr_ypreq_key;
#else
		__xdr_argument = (xdrproc_t) __xdr_ypreq_nokey;
#endif
		__xdr_result = (xdrproc_t) __xdr_ypresp_key_val;
		local = (char *(*)(char *, struct svc_req *)) ypproc_first_2_svc;
		break;

	case YPPROC_NEXT:
		__xdr_argument = (xdrproc_t) __xdr_ypreq_key;
		__xdr_result = (xdrproc_t) __xdr_ypresp_key_val;
		local = (char *(*)(char *, struct svc_req *)) ypproc_next_2_svc;
		break;

	case YPPROC_XFR:
		__xdr_argument = (xdrproc_t) __xdr_ypreq_xfr;
		__xdr_result = (xdrproc_t) __xdr_ypresp_xfr;
		local = (char *(*)(char *, struct svc_req *)) ypproc_xfr_2_svc;
		break;

	case YPPROC_CLEAR:
		__xdr_argument = (xdrproc_t) xdr_void;
		__xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) ypproc_clear_2_svc;
		break;

	case YPPROC_ALL:
		__xdr_argument = (xdrproc_t) __xdr_ypreq_nokey;
		__xdr_result = (xdrproc_t) __xdr_ypresp_all;
		local = (char *(*)(char *, struct svc_req *)) ypproc_all_2_svc;
		break;

	case YPPROC_MASTER:
		__xdr_argument = (xdrproc_t) __xdr_ypreq_nokey;
		__xdr_result = (xdrproc_t) __xdr_ypresp_master;
		local = (char *(*)(char *, struct svc_req *)) ypproc_master_2_svc;
		break;

	case YPPROC_ORDER:
		__xdr_argument = (xdrproc_t) __xdr_ypreq_nokey;
		__xdr_result = (xdrproc_t) __xdr_ypresp_order;
		local = (char *(*)(char *, struct svc_req *)) ypproc_order_2_svc;
		break;

	case YPPROC_MAPLIST:
		__xdr_argument = (xdrproc_t) __xdr_domainname;
		__xdr_result = (xdrproc_t) __xdr_ypresp_maplist;
		local = (char *(*)(char *, struct svc_req *)) ypproc_maplist_2_svc;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, __xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, __xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, __xdr_argument, (caddr_t) &argument)) {
		fprintf(stderr, "unable to free arguments");
		exit(1);
	}
	return;
}

#if 0
static void
yppush_xfrrespprog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		int fill;
	} argument;
	char *result;
	xdrproc_t __xdr_argument, __xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case YPPUSHPROC_NULL:
		__xdr_argument = (xdrproc_t) xdr_void;
		__xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) yppushproc_null_1_svc;
		break;

	case YPPUSHPROC_XFRRESP:
		__xdr_argument = (xdrproc_t) xdr_void;
		__xdr_result = (xdrproc_t) __xdr_yppushresp_xfr;
		local = (char *(*)(char *, struct svc_req *)) yppushproc_xfrresp_1_svc;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, __xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, __xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, __xdr_argument, (caddr_t) &argument)) {
		fprintf(stderr, "unable to free arguments");
		exit(1);
	}
	return;
}

static void
ypbindprog_2(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		domainname ypbindproc_domain_2_arg;
		ypbind_setdom ypbindproc_setdom_2_arg;
	} argument;
	char *result;
	xdrproc_t __xdr_argument, __xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case YPBINDPROC_NULL:
		__xdr_argument = (xdrproc_t) xdr_void;
		__xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) ypbindproc_null_2_svc;
		break;

	case YPBINDPROC_DOMAIN:
		__xdr_argument = (xdrproc_t) __xdr_domainname;
		__xdr_result = (xdrproc_t) __xdr_ypbind_resp;
		local = (char *(*)(char *, struct svc_req *)) ypbindproc_domain_2_svc;
		break;

	case YPBINDPROC_SETDOM:
		__xdr_argument = (xdrproc_t) __xdr_ypbind_setdom;
		__xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) ypbindproc_setdom_2_svc;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, __xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, __xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, __xdr_argument, (caddr_t) &argument)) {
		fprintf(stderr, "unable to free arguments");
		exit(1);
	}
	return;
}
#endif

extern int debug_flag;
extern int dns_flag;

#ifndef _PATH_YP
#define _PATH_YP   "/var/yp"
#endif
char *path_ypdb = _PATH_YP;

char *progname;


int main(int argc, char **argv)
{
    register SVCXPRT	*transp;
    int	         	i;
    int			my_port = -1;
    int			my_socket;
    struct sockaddr_in	socket_address;
    int			result;

    
    progname = strrchr (argv[0], '/');
    if (progname == (char *) NULL)
	progname = argv[0];
    else
	progname++;

    openlog(progname, LOG_PID, TCPW_FACILITY);
    
    for (i = 1; i < argc && argv[i][0] == '-'; i++)
    {
	if (strcmp(argv[i], "-debug") == 0 || strcmp(argv[i], "-d") == 0)
	    debug_flag = 1;
	else if (strcmp(argv[i], "-dns") == 0 || strcmp(argv[i], "-b") == 0)
	    dns_flag = 1;
	else if ((argv[i][1] == 'p') && (argv[i][2] >= '0') && (argv[i][2] <= '9'))
	    my_port = atoi(argv[i] + 2);
	else
	{
	    fprintf(stderr, "%s: Unknown command line switch: %s\n",
		    progname,
		    argv[i]);
	    exit(1);
	}
    }

    if (!debug_flag)
	    if(daemon(0,0))
	    {
		perror("daemon()");
		exit (1);
	    }

    if (debug_flag)
	Perror("[Welcome to the NYS YP Server, version 0.11]\n");

    if (i < argc)
    {
	path_ypdb = argv[i];
	if (debug_flag)
	    Perror("Using database directory: %s\n", path_ypdb);
    }

    /* Change current directory to database location */
    if (chdir(path_ypdb) < 0)
    {
	Perror("%s: chdir: %", argv[0], strerror(errno));
	exit(1);
    }
	
    (void) pmap_unset(YPPROG, YPVERS);

    if (my_port >= 0)
    {
	my_socket = socket (AF_INET, SOCK_DGRAM, 0);
	if (my_socket < 0)
	{
	    Perror("%s: can not create UDP: %s",
		     progname, strerror(errno));
	    exit (1);
	}

	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = htonl (INADDR_ANY);
	socket_address.sin_port = htons (my_port);

	result = bind (my_socket, (struct sockaddr *) &socket_address,
		       sizeof (socket_address));
	if (result < 0)
	{
	    Perror("%s: can not create UDP: %s",
		     progname, strerror(errno));
	    exit (1);
	}
    }
    else
	my_socket = RPC_ANYSOCK;

    transp = svcudp_create(my_socket);
    if (transp == NULL) {
	Perror("cannot create udp service.");
	exit(1);
    }
    if (!svc_register(transp, YPPROG, YPVERS, ypprog_2, IPPROTO_UDP)) {
	Perror("unable to register (YPPROG, YPVERS, udp).");
	exit(1);
    }

    if (my_port >= 0)
    {
	my_socket = socket (AF_INET, SOCK_STREAM, 0);
	if (my_socket < 0)
	{
	    Perror("%s: can not create TCP: %s",
		     progname, strerror(errno));
	    exit (1);
	}

	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = htonl (INADDR_ANY);
	socket_address.sin_port = htons (my_port);

	result = bind (my_socket, (struct sockaddr *) &socket_address,
		       sizeof (socket_address));
	if (result < 0)
	{
	    Perror("%s: can not create TCP: %s",
		     progname, strerror(errno));
	    exit (1);
	}
    }
    else
	my_socket = RPC_ANYSOCK;

    transp = svctcp_create(my_socket, 0, 0);
    if (transp == NULL) {
	Perror("%s: cannot create tcp service\n", progname);
	exit(1);
    }
    if (!svc_register(transp, YPPROG, YPVERS, ypprog_2, IPPROTO_TCP)) {
	Perror("%s: unable to register (YPPROG, YPVERS, tcp)\n", progname);
	exit(1);
    }
	
    svc_run();
    Perror("svc_run returned");
    exit(1);
    /* NOTREACHED */
}
