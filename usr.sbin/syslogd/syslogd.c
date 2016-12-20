/*
 * Copyright (c) 1983, 1988, 1993, 1994
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

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1983, 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)syslogd.c	8.3 (Berkeley) 4/4/94";
#endif
#endif /* not lint */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as "<n>" as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximum line length that can be handled.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 * more extensive changes by Eric Allman (again)
 * Extension to log by program name as well as facility and priority
 *   by Peter da Silva.
 * -u and -v by Harlan Stenn.
 * Priority comparison code by Harlan Stenn.
 */

#define	MAXLINE		1024		/* maximum line length */
#define	MAXSVLINE	MAXLINE		/* maximum saved line length */
#define	DEFUPRI		(LOG_USER|LOG_NOTICE)
#define	DEFSPRI		(LOG_KERN|LOG_CRIT)
#define	TIMERINTVL	30		/* interval for checking flush, mark */
#define	TTYMSGTIME	1		/* timeout passed to ttymsg */
#define	RCVBUF_MINSIZE	(80 * 1024)	/* minimum size of dgram rcv buffer */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syslimits.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libutil.h>
#include <limits.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <utmpx.h>

#include "pathnames.h"
#include "ttymsg.h"

#define SYSLOG_NAMES
#include <sys/syslog.h>

static const char *ConfFile = _PATH_LOGCONF;
static const char *PidFile = _PATH_LOGPID;
static const char ctty[] = _PATH_CONSOLE;
static const char include_str[] = "include";
static const char include_ext[] = ".conf";

#define	dprintf		if (Debug) printf

#define	MAXUNAMES	20	/* maximum number of user names */

#define	sstosa(ss)	((struct sockaddr *)(ss))
#define	satosin6(sa)	((struct sockaddr_in6 *)(void *)(sa))
#define	satosin(sa)	((struct sockaddr_in *)(void *)(sa))
#define	s6_addr32	__u6_addr.__u6_addr32
#define	IN6_ARE_MASKED_ADDR_EQUAL(d, a, m)	(	\
	(((d)->s6_addr32[0] ^ (a)->s6_addr32[0]) & (m)->s6_addr32[0]) == 0 && \
	(((d)->s6_addr32[1] ^ (a)->s6_addr32[1]) & (m)->s6_addr32[1]) == 0 && \
	(((d)->s6_addr32[2] ^ (a)->s6_addr32[2]) & (m)->s6_addr32[2]) == 0 && \
	(((d)->s6_addr32[3] ^ (a)->s6_addr32[3]) & (m)->s6_addr32[3]) == 0 )
/*
 * List of peers and sockets for binding.
 */
struct peer {
	const char	*pe_name;
	const char	*pe_serv;
	mode_t		pe_mode;
	STAILQ_ENTRY(peer)	next;
};
struct socklist {
	struct sockaddr_storage	sl_ss;
	int			sl_socket;
	struct peer		*sl_peer;
	STAILQ_ENTRY(socklist)	next;
};
static STAILQ_HEAD(, socklist) shead = STAILQ_HEAD_INITIALIZER(shead);

static struct peer funix_secure = {
	.pe_name = _PATH_LOG_PRIV,
	.pe_mode = S_IRUSR | S_IWUSR,
	.next = {NULL},
};
static struct peer funix_default = {
	.pe_name = _PATH_LOG,
	.pe_mode = DEFFILEMODE,
	.next = {&funix_secure},
};
static STAILQ_HEAD(, peer) pqueue = {
	&funix_default,
	&funix_secure.next.stqe_next,
};

/*
 * Flags to logmsg().
 */

#define	IGN_CONS	0x001	/* don't print on console */
#define	SYNC_FILE	0x002	/* do fsync on file after printing */
#define	ADDDATE		0x004	/* add a date to the message */
#define	MARK		0x008	/* this message is a mark */
#define	ISKERNEL	0x010	/* kernel generated message */

/*
 * This structure represents the files that will have log
 * copies printed.
 * We require f_file to be valid if f_type is F_FILE, F_CONSOLE, F_TTY
 * or if f_type if F_PIPE and f_pid > 0.
 */

struct filed {
	STAILQ_ENTRY(filed)	next;	/* next in linked list */
	short	f_type;			/* entry type, see below */
	short	f_file;			/* file descriptor */
	time_t	f_time;			/* time this was last written */
	char	*f_host;		/* host from which to recd. */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	u_char	f_pcmp[LOG_NFACILITIES+1];	/* compare priority */
#define PRI_LT	0x1
#define PRI_EQ	0x2
#define PRI_GT	0x4
	char	*f_program;		/* program this applies to */
	union {
		char	f_uname[MAXUNAMES][MAXLOGNAME];
		struct {
			char	f_hname[MAXHOSTNAMELEN];
			struct addrinfo *f_addr;

		} f_forw;		/* forwarding address */
		char	f_fname[MAXPATHLEN];
		struct {
			char	f_pname[MAXPATHLEN];
			pid_t	f_pid;
		} f_pipe;
	} f_un;
#define	fu_uname	f_un.f_uname
#define	fu_forw_hname	f_un.f_forw.f_hname
#define	fu_forw_addr	f_un.f_forw.f_addr
#define	fu_fname	f_un.f_fname
#define	fu_pipe_pname	f_un.f_pipe.f_pname
#define	fu_pipe_pid	f_un.f_pipe.f_pid
	char	f_prevline[MAXSVLINE];		/* last message logged */
	char	f_lasttime[16];			/* time of last occurrence */
	char	f_prevhost[MAXHOSTNAMELEN];	/* host from which recd. */
	int	f_prevpri;			/* pri of f_prevline */
	int	f_prevlen;			/* length of f_prevline */
	int	f_prevcount;			/* repetition cnt of prevline */
	u_int	f_repeatcount;			/* number of "repeated" msgs */
	int	f_flags;			/* file-specific flags */
#define	FFLAG_SYNC 0x01
#define	FFLAG_NEEDSYNC	0x02
};

/*
 * Queue of about-to-be dead processes we should watch out for.
 */
struct deadq_entry {
	pid_t				dq_pid;
	int				dq_timeout;
	TAILQ_ENTRY(deadq_entry)	dq_entries;
};
static TAILQ_HEAD(, deadq_entry) deadq_head;

/*
 * The timeout to apply to processes waiting on the dead queue.  Unit
 * of measure is `mark intervals', i.e. 20 minutes by default.
 * Processes on the dead queue will be terminated after that time.
 */

#define	 DQ_TIMO_INIT	2

typedef struct deadq_entry *dq_t;


/*
 * Struct to hold records of network addresses that are allowed to log
 * to us.
 */
struct allowedpeer {
	int isnumeric;
	u_short port;
	union {
		struct {
			struct sockaddr_storage addr;
			struct sockaddr_storage mask;
		} numeric;
		char *name;
	} u;
#define a_addr u.numeric.addr
#define a_mask u.numeric.mask
#define a_name u.name
	STAILQ_ENTRY(allowedpeer)	next;
};
static STAILQ_HEAD(, allowedpeer) aphead = STAILQ_HEAD_INITIALIZER(aphead);


/*
 * Intervals at which we flush out "message repeated" messages,
 * in seconds after previous message is logged.  After each flush,
 * we move to the next interval until we reach the largest.
 */
static int repeatinterval[] = { 30, 120, 600 };	/* # of secs before flush */
#define	MAXREPEAT ((sizeof(repeatinterval) / sizeof(repeatinterval[0])) - 1)
#define	REPEATTIME(f)	((f)->f_time + repeatinterval[(f)->f_repeatcount])
#define	BACKOFF(f)	{ if (++(f)->f_repeatcount > MAXREPEAT) \
				 (f)->f_repeatcount = MAXREPEAT; \
			}

/* values for f_type */
#define F_UNUSED	0		/* unused entry */
#define F_FILE		1		/* regular file */
#define F_TTY		2		/* terminal */
#define F_CONSOLE	3		/* console terminal */
#define F_FORW		4		/* remote machine */
#define F_USERS		5		/* list of users */
#define F_WALL		6		/* everyone logged on */
#define F_PIPE		7		/* pipe to program */

static const char *TypeNames[8] = {
	"UNUSED",	"FILE",		"TTY",		"CONSOLE",
	"FORW",		"USERS",	"WALL",		"PIPE"
};

static STAILQ_HEAD(, filed) fhead =
    STAILQ_HEAD_INITIALIZER(fhead);	/* Log files that we write to */
static struct filed consfile;	/* Console */

static int	Debug;		/* debug flag */
static int	Foreground = 0;	/* Run in foreground, instead of daemonizing */
static int	resolve = 1;	/* resolve hostname */
static char	LocalHostName[MAXHOSTNAMELEN];	/* our hostname */
static const char *LocalDomain;	/* our local domain name */
static int	fklog = -1;	/* /dev/klog */
static int	Initialized;	/* set when we have initialized ourselves */
static int	MarkInterval = 20 * 60;	/* interval between marks in seconds */
static int	MarkSeq;	/* mark sequence number */
static int	NoBind;		/* don't bind() as suggested by RFC 3164 */
static int	SecureMode;	/* when true, receive only unix domain socks */
#ifdef INET6
static int	family = PF_UNSPEC; /* protocol family (IPv4, IPv6 or both) */
#else
static int	family = PF_INET; /* protocol family (IPv4 only) */
#endif
static int	mask_C1 = 1;	/* mask characters from 0x80 - 0x9F */
static int	send_to_all;	/* send message to all IPv4/IPv6 addresses */
static int	use_bootfile;	/* log entire bootfile for every kern msg */
static int	no_compress;	/* don't compress messages (1=pipes, 2=all) */
static int	logflags = O_WRONLY|O_APPEND; /* flags used to open log files */

static char	bootfile[MAXLINE+1]; /* booted kernel file */

static int	RemoteAddDate;	/* Always set the date on remote messages */

static int	UniquePriority;	/* Only log specified priority? */
static int	LogFacPri;	/* Put facility and priority in log message: */
				/* 0=no, 1=numeric, 2=names */
static int	KeepKernFac;	/* Keep remotely logged kernel facility */
static int	needdofsync = 0; /* Are any file(s) waiting to be fsynced? */
static struct pidfh *pfh;

static volatile sig_atomic_t MarkSet, WantDie;

static int	allowaddr(char *);
static void	cfline(const char *, struct filed *,
		    const char *, const char *);
static const char *cvthname(struct sockaddr *);
static void	deadq_enter(pid_t, const char *);
static int	deadq_remove(pid_t);
static int	decode(const char *, const CODE *);
static void	die(int) __dead2;
static void	dodie(int);
static void	dofsync(void);
static void	domark(int);
static void	fprintlog(struct filed *, int, const char *);
static void	init(int);
static void	logerror(const char *);
static void	logmsg(int, const char *, const char *, int);
static void	log_deadchild(pid_t, int, const char *);
static void	markit(void);
static int	socksetup(struct peer *);
static int	skip_message(const char *, const char *, int);
static void	printline(const char *, char *, int);
static void	printsys(char *);
static int	p_open(const char *, pid_t *);
static void	readklog(void);
static void	reapchild(int);
static const char *ttymsg_check(struct iovec *, int, char *, int);
static void	usage(void);
static int	validate(struct sockaddr *, const char *);
static void	unmapped(struct sockaddr *);
static void	wallmsg(struct filed *, struct iovec *, const int iovlen);
static int	waitdaemon(int, int, int);
static void	timedout(int);
static void	increase_rcvbuf(int);

static void
close_filed(struct filed *f)
{

	if (f == NULL || f->f_file == -1)
		return;

	(void)close(f->f_file);
	f->f_file = -1;
	f->f_type = F_UNUSED;
}

int
main(int argc, char *argv[])
{
	int ch, i, fdsrmax = 0, bflag = 0;
	struct sockaddr_storage ss;
	fd_set *fdsr = NULL;
	char line[MAXLINE + 1];
	const char *hname;
	struct timeval tv, *tvp;
	struct sigaction sact;
	struct peer *pe;
	struct socklist *sl;
	sigset_t mask;
	pid_t ppid = 1, spid;
	socklen_t sslen;
	char *p;

	if (madvise(NULL, 0, MADV_PROTECT) != 0)
		dprintf("madvise() failed: %s\n", strerror(errno));

	while ((ch = getopt(argc, argv, "468Aa:b:cCdf:Fkl:m:nNop:P:sS:Tuv"))
	    != -1)
		switch (ch) {
#ifdef INET
		case '4':
			family = PF_INET;
			break;
#endif
#ifdef INET6
		case '6':
			family = PF_INET6;
			break;
#endif
		case '8':
			mask_C1 = 0;
			break;
		case 'A':
			send_to_all++;
			break;
		case 'a':		/* allow specific network addresses only */
			if (allowaddr(optarg) == -1)
				usage();
			break;
		case 'b':
			if ((pe = calloc(1, sizeof(*pe))) == NULL)
				err(1, "malloc failed");
			if ((p = strchr(optarg, ':')) == NULL) {
				/* A hostname or filename only. */
				pe->pe_name = optarg;
				pe->pe_serv = "syslog";
			} else {
				/* The case of "name:service". */
				*p++ = '\0';
				pe->pe_serv = p;
				pe->pe_name = (strlen(optarg) == 0) ?
				    NULL : optarg;
			}
			bflag = 1;
			STAILQ_INSERT_TAIL(&pqueue, pe, next);
			break;
		case 'c':
			no_compress++;
			break;
		case 'C':
			logflags |= O_CREAT;
			break;
		case 'd':		/* debug */
			Debug++;
			break;
		case 'f':		/* configuration file */
			ConfFile = optarg;
			break;
		case 'F':		/* run in foreground instead of daemon */
			Foreground++;
			break;
		case 'k':		/* keep remote kern fac */
			KeepKernFac = 1;
			break;
		case 'l':
		    {
			long	perml;
			mode_t	mode;
			char	*name, *ep;

			if (optarg[0] == '/') {
				mode = DEFFILEMODE;
				name = optarg;
			} else if ((name = strchr(optarg, ':')) != NULL) {
				*name++ = '\0';
				if (name[0] != '/')
					errx(1, "socket name must be absolute "
					    "path");
				if (isdigit(*optarg)) {
					perml = strtol(optarg, &ep, 8);
				    if (*ep || perml < 0 ||
					perml & ~(S_IRWXU|S_IRWXG|S_IRWXO))
					    errx(1, "invalid mode %s, exiting",
						optarg);
				    mode = (mode_t )perml;
				} else
					errx(1, "invalid mode %s, exiting",
					    optarg);
			}
			if ((pe = calloc(1, sizeof(*pe))) == NULL)
				err(1, "malloc failed");
			pe->pe_name = name;
			pe->pe_mode = mode;
			STAILQ_INSERT_TAIL(&pqueue, pe, next);
			break;
		   }
		case 'm':		/* mark interval */
			MarkInterval = atoi(optarg) * 60;
			break;
		case 'N':
			NoBind = 1;
			SecureMode = 1;
			break;
		case 'n':
			resolve = 0;
			break;
		case 'o':
			use_bootfile = 1;
			break;
		case 'p':		/* path */
			funix_default.pe_name = optarg;
			break;
		case 'P':		/* path for alt. PID */
			PidFile = optarg;
			break;
		case 's':		/* no network mode */
			SecureMode++;
			break;
		case 'S':		/* path for privileged originator */
			funix_secure.pe_name = optarg;
			break;
		case 'T':
			RemoteAddDate = 1;
			break;
		case 'u':		/* only log specified priority */
			UniquePriority++;
			break;
		case 'v':		/* log facility and priority */
		  	LogFacPri++;
			break;
		default:
			usage();
		}
	if ((argc -= optind) != 0)
		usage();

	if (bflag == 0) {
		pe = calloc(1, sizeof(*pe));
		*pe = (struct peer) {
			.pe_serv = "syslog"
		};
		STAILQ_INSERT_TAIL(&pqueue, pe, next);
	}
	STAILQ_FOREACH(pe, &pqueue, next)
		socksetup(pe);

	pfh = pidfile_open(PidFile, 0600, &spid);
	if (pfh == NULL) {
		if (errno == EEXIST)
			errx(1, "syslogd already running, pid: %d", spid);
		warn("cannot open pid file");
	}

	if ((!Foreground) && (!Debug)) {
		ppid = waitdaemon(0, 0, 30);
		if (ppid < 0) {
			warn("could not become daemon");
			pidfile_remove(pfh);
			exit(1);
		}
	} else if (Debug)
		setlinebuf(stdout);

	consfile.f_type = F_CONSOLE;
	(void)strlcpy(consfile.fu_fname, ctty + sizeof _PATH_DEV - 1,
	    sizeof(consfile.fu_fname));
	(void)strlcpy(bootfile, getbootfile(), sizeof(bootfile));
	(void)signal(SIGTERM, dodie);
	(void)signal(SIGINT, Debug ? dodie : SIG_IGN);
	(void)signal(SIGQUIT, Debug ? dodie : SIG_IGN);
	/*
	 * We don't want the SIGCHLD and SIGHUP handlers to interfere
	 * with each other; they are likely candidates for being called
	 * simultaneously (SIGHUP closes pipe descriptor, process dies,
	 * SIGCHLD happens).
	 */
	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);
	sact.sa_handler = reapchild;
	sact.sa_mask = mask;
	sact.sa_flags = SA_RESTART;
	(void)sigaction(SIGCHLD, &sact, NULL);
	(void)signal(SIGALRM, domark);
	(void)signal(SIGPIPE, SIG_IGN);	/* We'll catch EPIPE instead. */
	(void)alarm(TIMERINTVL);

	TAILQ_INIT(&deadq_head);

	if ((fklog = open(_PATH_KLOG, O_RDONLY|O_NONBLOCK, 0)) < 0)
		dprintf("can't open %s (%d)\n", _PATH_KLOG, errno);

	/* tuck my process id away */
	pidfile_write(pfh);

	dprintf("off & running....\n");

	init(0);
	/* prevent SIGHUP and SIGCHLD handlers from running in parallel */
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sact.sa_handler = init;
	sact.sa_mask = mask;
	sact.sa_flags = SA_RESTART;
	(void)sigaction(SIGHUP, &sact, NULL);

	tvp = &tv;
	tv.tv_sec = tv.tv_usec = 0;

	if (fklog != -1 && fklog > fdsrmax)
		fdsrmax = fklog;
	STAILQ_FOREACH(sl, &shead, next) {
		if (sl->sl_socket > fdsrmax)
			fdsrmax = sl->sl_socket;
	}

	fdsr = (fd_set *)calloc(howmany(fdsrmax+1, NFDBITS),
	    sizeof(fd_mask));
	if (fdsr == NULL)
		errx(1, "calloc fd_set");

	for (;;) {
		if (MarkSet)
			markit();
		if (WantDie)
			die(WantDie);

		bzero(fdsr, howmany(fdsrmax+1, NFDBITS) *
		    sizeof(fd_mask));

		if (fklog != -1)
			FD_SET(fklog, fdsr);
		STAILQ_FOREACH(sl, &shead, next) {
			if (sl->sl_socket != -1)
				FD_SET(sl->sl_socket, fdsr);
		}
		i = select(fdsrmax + 1, fdsr, NULL, NULL,
		    needdofsync ? &tv : tvp);
		switch (i) {
		case 0:
			dofsync();
			needdofsync = 0;
			if (tvp) {
				tvp = NULL;
				if (ppid != 1)
					kill(ppid, SIGALRM);
			}
			continue;
		case -1:
			if (errno != EINTR)
				logerror("select");
			continue;
		}
		if (fklog != -1 && FD_ISSET(fklog, fdsr))
			readklog();
		STAILQ_FOREACH(sl, &shead, next) {
			int date, len;

			if (FD_ISSET(sl->sl_socket, fdsr)) {
				sslen = sizeof(ss);
				dprintf("sslen(1) = %d\n", sslen);
				len = recvfrom(sl->sl_socket, line,
				    sizeof(line) - 1, 0,
				    sstosa(&ss), &sslen);
				dprintf("sslen(2) = %d\n", sslen);
				if (len == 0)
					continue;
				if (len < 0) {
					if (errno != EINTR)
						logerror("recvfrom");
					continue;
				}
				/* Received valid data. */
				line[len] = '\0';
				if (sl->sl_ss.ss_family == AF_LOCAL) {
					hname = LocalHostName;
					date = 0;
				} else {
					hname = cvthname(sstosa(&ss));
					unmapped(sstosa(&ss));
					if (validate(sstosa(&ss), hname) == 0)
						hname = NULL;
					date = RemoteAddDate ? ADDDATE : 0;
				}
				if (hname != NULL)
					printline(hname, line, date);
				else
					dprintf("Invalid msg from "
					    "%s was ignored.", hname);
			}
		}
	}
	if (fdsr)
		free(fdsr);
}

static void
unmapped(struct sockaddr *sa)
{
	struct sockaddr_in6 *sin6;
	struct sockaddr_in sin4;

	if (sa == NULL ||
	    sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(*sin6))
		return;
	sin6 = satosin6(sa);
	if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
		return;

	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
	sin4.sin_len = sizeof(sin4);
	memcpy(&sin4.sin_addr, &sin6->sin6_addr.s6_addr[12],
	       sizeof(sin4.sin_addr));
	sin4.sin_port = sin6->sin6_port;

	memcpy(sa, &sin4, sin4.sin_len);
}

static void
usage(void)
{

	fprintf(stderr, "%s\n%s\n%s\n%s\n",
		"usage: syslogd [-468ACcdFknosTuv] [-a allowed_peer]",
		"               [-b bind_address] [-f config_file]",
		"               [-l [mode:]path] [-m mark_interval]",
		"               [-P pid_file] [-p log_socket]");
	exit(1);
}

/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */
static void
printline(const char *hname, char *msg, int flags)
{
	char *p, *q;
	long n;
	int c, pri;
	char line[MAXLINE + 1];

	/* test for special codes */
	p = msg;
	pri = DEFUPRI;
	if (*p == '<') {
		errno = 0;
		n = strtol(p + 1, &q, 10);
		if (*q == '>' && n >= 0 && n < INT_MAX && errno == 0) {
			p = q + 1;
			pri = n;
		}
	}
	if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
		pri = DEFUPRI;

	/*
	 * Don't allow users to log kernel messages.
	 * NOTE: since LOG_KERN == 0 this will also match
	 *       messages with no facility specified.
	 */
	if ((pri & LOG_FACMASK) == LOG_KERN && !KeepKernFac)
		pri = LOG_MAKEPRI(LOG_USER, LOG_PRI(pri));

	q = line;

	while ((c = (unsigned char)*p++) != '\0' &&
	    q < &line[sizeof(line) - 4]) {
		if (mask_C1 && (c & 0x80) && c < 0xA0) {
			c &= 0x7F;
			*q++ = 'M';
			*q++ = '-';
		}
		if (isascii(c) && iscntrl(c)) {
			if (c == '\n') {
				*q++ = ' ';
			} else if (c == '\t') {
				*q++ = '\t';
			} else {
				*q++ = '^';
				*q++ = c ^ 0100;
			}
		} else {
			*q++ = c;
		}
	}
	*q = '\0';

	logmsg(pri, line, hname, flags);
}

/*
 * Read /dev/klog while data are available, split into lines.
 */
static void
readklog(void)
{
	char *p, *q, line[MAXLINE + 1];
	int len, i;

	len = 0;
	for (;;) {
		i = read(fklog, line + len, MAXLINE - 1 - len);
		if (i > 0) {
			line[i + len] = '\0';
		} else {
			if (i < 0 && errno != EINTR && errno != EAGAIN) {
				logerror("klog");
				fklog = -1;
			}
			break;
		}

		for (p = line; (q = strchr(p, '\n')) != NULL; p = q + 1) {
			*q = '\0';
			printsys(p);
		}
		len = strlen(p);
		if (len >= MAXLINE - 1) {
			printsys(p);
			len = 0;
		}
		if (len > 0)
			memmove(line, p, len + 1);
	}
	if (len > 0)
		printsys(line);
}

/*
 * Take a raw input line from /dev/klog, format similar to syslog().
 */
static void
printsys(char *msg)
{
	char *p, *q;
	long n;
	int flags, isprintf, pri;

	flags = ISKERNEL | SYNC_FILE | ADDDATE;	/* fsync after write */
	p = msg;
	pri = DEFSPRI;
	isprintf = 1;
	if (*p == '<') {
		errno = 0;
		n = strtol(p + 1, &q, 10);
		if (*q == '>' && n >= 0 && n < INT_MAX && errno == 0) {
			p = q + 1;
			pri = n;
			isprintf = 0;
		}
	}
	/*
	 * Kernel printf's and LOG_CONSOLE messages have been displayed
	 * on the console already.
	 */
	if (isprintf || (pri & LOG_FACMASK) == LOG_CONSOLE)
		flags |= IGN_CONS;
	if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
		pri = DEFSPRI;
	logmsg(pri, p, LocalHostName, flags);
}

static time_t	now;

/*
 * Match a program or host name against a specification.
 * Return a non-0 value if the message must be ignored
 * based on the specification.
 */
static int
skip_message(const char *name, const char *spec, int checkcase)
{
	const char *s;
	char prev, next;
	int exclude = 0;
	/* Behaviour on explicit match */

	if (spec == NULL)
		return 0;
	switch (*spec) {
	case '-':
		exclude = 1;
		/*FALLTHROUGH*/
	case '+':
		spec++;
		break;
	default:
		break;
	}
	if (checkcase)
		s = strstr (spec, name);
	else
		s = strcasestr (spec, name);

	if (s != NULL) {
		prev = (s == spec ? ',' : *(s - 1));
		next = *(s + strlen (name));

		if (prev == ',' && (next == '\0' || next == ','))
			/* Explicit match: skip iff the spec is an
			   exclusive one. */
			return exclude;
	}

	/* No explicit match for this name: skip the message iff
	   the spec is an inclusive one. */
	return !exclude;
}

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */
static void
logmsg(int pri, const char *msg, const char *from, int flags)
{
	struct filed *f;
	int i, fac, msglen, omask, prilev;
	const char *timestamp;
 	char prog[NAME_MAX+1];
	char buf[MAXLINE+1];

	dprintf("logmsg: pri %o, flags %x, from %s, msg %s\n",
	    pri, flags, from, msg);

	omask = sigblock(sigmask(SIGHUP)|sigmask(SIGALRM));

	/*
	 * Check to see if msg looks non-standard.
	 */
	msglen = strlen(msg);
	if (msglen < 16 || msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ')
		flags |= ADDDATE;

	(void)time(&now);
	if (flags & ADDDATE) {
		timestamp = ctime(&now) + 4;
	} else {
		timestamp = msg;
		msg += 16;
		msglen -= 16;
	}

	/* skip leading blanks */
	while (isspace(*msg)) {
		msg++;
		msglen--;
	}

	/* extract facility and priority level */
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	else
		fac = LOG_FAC(pri);

	/* Check maximum facility number. */
	if (fac > LOG_NFACILITIES) {
		(void)sigsetmask(omask);
		return;
	}

	prilev = LOG_PRI(pri);

	/* extract program name */
	for (i = 0; i < NAME_MAX; i++) {
		if (!isprint(msg[i]) || msg[i] == ':' || msg[i] == '[' ||
		    msg[i] == '/' || isspace(msg[i]))
			break;
		prog[i] = msg[i];
	}
	prog[i] = 0;

	/* add kernel prefix for kernel messages */
	if (flags & ISKERNEL) {
		snprintf(buf, sizeof(buf), "%s: %s",
		    use_bootfile ? bootfile : "kernel", msg);
		msg = buf;
		msglen = strlen(buf);
	}

	/* log the message to the particular outputs */
	if (!Initialized) {
		f = &consfile;
		/*
		 * Open in non-blocking mode to avoid hangs during open
		 * and close(waiting for the port to drain).
		 */
		f->f_file = open(ctty, O_WRONLY | O_NONBLOCK, 0);

		if (f->f_file >= 0) {
			(void)strlcpy(f->f_lasttime, timestamp,
				sizeof(f->f_lasttime));
			fprintlog(f, flags, msg);
			close(f->f_file);
			f->f_file = -1;
		}
		(void)sigsetmask(omask);
		return;
	}
	STAILQ_FOREACH(f, &fhead, next) {
		/* skip messages that are incorrect priority */
		if (!(((f->f_pcmp[fac] & PRI_EQ) && (f->f_pmask[fac] == prilev))
		     ||((f->f_pcmp[fac] & PRI_LT) && (f->f_pmask[fac] < prilev))
		     ||((f->f_pcmp[fac] & PRI_GT) && (f->f_pmask[fac] > prilev))
		     )
		    || f->f_pmask[fac] == INTERNAL_NOPRI)
			continue;

		/* skip messages with the incorrect hostname */
		if (skip_message(from, f->f_host, 0))
			continue;

		/* skip messages with the incorrect program name */
		if (skip_message(prog, f->f_program, 1))
			continue;

		/* skip message to console if it has already been printed */
		if (f->f_type == F_CONSOLE && (flags & IGN_CONS))
			continue;

		/* don't output marks to recently written files */
		if ((flags & MARK) && (now - f->f_time) < MarkInterval / 2)
			continue;

		/*
		 * suppress duplicate lines to this file
		 */
		if (no_compress - (f->f_type != F_PIPE) < 1 &&
		    (flags & MARK) == 0 && msglen == f->f_prevlen &&
		    !strcmp(msg, f->f_prevline) &&
		    !strcasecmp(from, f->f_prevhost)) {
			(void)strlcpy(f->f_lasttime, timestamp,
				sizeof(f->f_lasttime));
			f->f_prevcount++;
			dprintf("msg repeated %d times, %ld sec of %d\n",
			    f->f_prevcount, (long)(now - f->f_time),
			    repeatinterval[f->f_repeatcount]);
			/*
			 * If domark would have logged this by now,
			 * flush it now (so we don't hold isolated messages),
			 * but back off so we'll flush less often
			 * in the future.
			 */
			if (now > REPEATTIME(f)) {
				fprintlog(f, flags, (char *)NULL);
				BACKOFF(f);
			}
		} else {
			/* new line, save it */
			if (f->f_prevcount)
				fprintlog(f, 0, (char *)NULL);
			f->f_repeatcount = 0;
			f->f_prevpri = pri;
			(void)strlcpy(f->f_lasttime, timestamp,
				sizeof(f->f_lasttime));
			(void)strlcpy(f->f_prevhost, from,
			    sizeof(f->f_prevhost));
			if (msglen < MAXSVLINE) {
				f->f_prevlen = msglen;
				(void)strlcpy(f->f_prevline, msg, sizeof(f->f_prevline));
				fprintlog(f, flags, (char *)NULL);
			} else {
				f->f_prevline[0] = 0;
				f->f_prevlen = 0;
				fprintlog(f, flags, msg);
			}
		}
	}
	(void)sigsetmask(omask);
}

static void
dofsync(void)
{
	struct filed *f;

	STAILQ_FOREACH(f, &fhead, next) {
		if ((f->f_type == F_FILE) &&
		    (f->f_flags & FFLAG_NEEDSYNC)) {
			f->f_flags &= ~FFLAG_NEEDSYNC;
			(void)fsync(f->f_file);
		}
	}
}

#define IOV_SIZE 7
static void
fprintlog(struct filed *f, int flags, const char *msg)
{
	struct iovec iov[IOV_SIZE];
	struct iovec *v;
	struct addrinfo *r;
	int l, lsent = 0;
	char line[MAXLINE + 1], repbuf[80], greetings[200], *wmsg = NULL;
	char nul[] = "", space[] = " ", lf[] = "\n", crlf[] = "\r\n";
	const char *msgret;

	v = iov;
	if (f->f_type == F_WALL) {
		v->iov_base = greetings;
		/* The time displayed is not synchornized with the other log
		 * destinations (like messages).  Following fragment was using
		 * ctime(&now), which was updating the time every 30 sec.
		 * With f_lasttime, time is synchronized correctly.
		 */
		v->iov_len = snprintf(greetings, sizeof greetings,
		    "\r\n\7Message from syslogd@%s at %.24s ...\r\n",
		    f->f_prevhost, f->f_lasttime);
		if (v->iov_len >= sizeof greetings)
			v->iov_len = sizeof greetings - 1;
		v++;
		v->iov_base = nul;
		v->iov_len = 0;
		v++;
	} else {
		v->iov_base = f->f_lasttime;
		v->iov_len = strlen(f->f_lasttime);
		v++;
		v->iov_base = space;
		v->iov_len = 1;
		v++;
	}

	if (LogFacPri) {
	  	static char fp_buf[30];	/* Hollow laugh */
		int fac = f->f_prevpri & LOG_FACMASK;
		int pri = LOG_PRI(f->f_prevpri);
		const char *f_s = NULL;
		char f_n[5];	/* Hollow laugh */
		const char *p_s = NULL;
		char p_n[5];	/* Hollow laugh */

		if (LogFacPri > 1) {
		  const CODE *c;

		  for (c = facilitynames; c->c_name; c++) {
		    if (c->c_val == fac) {
		      f_s = c->c_name;
		      break;
		    }
		  }
		  for (c = prioritynames; c->c_name; c++) {
		    if (c->c_val == pri) {
		      p_s = c->c_name;
		      break;
		    }
		  }
		}
		if (!f_s) {
		  snprintf(f_n, sizeof f_n, "%d", LOG_FAC(fac));
		  f_s = f_n;
		}
		if (!p_s) {
		  snprintf(p_n, sizeof p_n, "%d", pri);
		  p_s = p_n;
		}
		snprintf(fp_buf, sizeof fp_buf, "<%s.%s> ", f_s, p_s);
		v->iov_base = fp_buf;
		v->iov_len = strlen(fp_buf);
	} else {
		v->iov_base = nul;
		v->iov_len = 0;
	}
	v++;

	v->iov_base = f->f_prevhost;
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = space;
	v->iov_len = 1;
	v++;

	if (msg) {
		wmsg = strdup(msg); /* XXX iov_base needs a `const' sibling. */
		if (wmsg == NULL) {
			logerror("strdup");
			exit(1);
		}
		v->iov_base = wmsg;
		v->iov_len = strlen(msg);
	} else if (f->f_prevcount > 1) {
		v->iov_base = repbuf;
		v->iov_len = snprintf(repbuf, sizeof repbuf,
		    "last message repeated %d times", f->f_prevcount);
	} else {
		v->iov_base = f->f_prevline;
		v->iov_len = f->f_prevlen;
	}
	v++;

	dprintf("Logging to %s", TypeNames[f->f_type]);
	f->f_time = now;

	switch (f->f_type) {
		int port;
	case F_UNUSED:
		dprintf("\n");
		break;

	case F_FORW:
		port = ntohs(satosin(f->fu_forw_addr->ai_addr)->sin_port);
		if (port != 514) {
			dprintf(" %s:%d\n", f->fu_forw_hname, port);
		} else {
			dprintf(" %s\n", f->fu_forw_hname);
		}
		/* check for local vs remote messages */
		if (strcasecmp(f->f_prevhost, LocalHostName))
			l = snprintf(line, sizeof line - 1,
			    "<%d>%.15s Forwarded from %s: %s",
			    f->f_prevpri, (char *)iov[0].iov_base,
			    f->f_prevhost, (char *)iov[5].iov_base);
		else
			l = snprintf(line, sizeof line - 1, "<%d>%.15s %s",
			     f->f_prevpri, (char *)iov[0].iov_base,
			    (char *)iov[5].iov_base);
		if (l < 0)
			l = 0;
		else if (l > MAXLINE)
			l = MAXLINE;

		for (r = f->fu_forw_addr; r; r = r->ai_next) {
			struct socklist *sl;

			STAILQ_FOREACH(sl, &shead, next) {
				if (sl->sl_ss.ss_family == AF_LOCAL)
					continue;
				lsent = sendto(sl->sl_socket, line, l, 0,
				    r->ai_addr, r->ai_addrlen);
				if (lsent == l)
					break;
			}
			if (lsent == l && !send_to_all)
				break;
		}
		dprintf("lsent/l: %d/%d\n", lsent, l);
		if (lsent != l) {
			int e = errno;
			logerror("sendto");
			errno = e;
			switch (errno) {
			case ENOBUFS:
			case ENETDOWN:
			case ENETUNREACH:
			case EHOSTUNREACH:
			case EHOSTDOWN:
			case EADDRNOTAVAIL:
				break;
			/* case EBADF: */
			/* case EACCES: */
			/* case ENOTSOCK: */
			/* case EFAULT: */
			/* case EMSGSIZE: */
			/* case EAGAIN: */
			/* case ENOBUFS: */
			/* case ECONNREFUSED: */
			default:
				dprintf("removing entry: errno=%d\n", e);
				f->f_type = F_UNUSED;
				break;
			}
		}
		break;

	case F_FILE:
		dprintf(" %s\n", f->fu_fname);
		v->iov_base = lf;
		v->iov_len = 1;
		if (writev(f->f_file, iov, IOV_SIZE) < 0) {
			/*
			 * If writev(2) fails for potentially transient errors
			 * like the filesystem being full, ignore it.
			 * Otherwise remove this logfile from the list.
			 */
			if (errno != ENOSPC) {
				int e = errno;
				close_filed(f);
				errno = e;
				logerror(f->fu_fname);
			}
		} else if ((flags & SYNC_FILE) && (f->f_flags & FFLAG_SYNC)) {
			f->f_flags |= FFLAG_NEEDSYNC;
			needdofsync = 1;
		}
		break;

	case F_PIPE:
		dprintf(" %s\n", f->fu_pipe_pname);
		v->iov_base = lf;
		v->iov_len = 1;
		if (f->fu_pipe_pid == 0) {
			if ((f->f_file = p_open(f->fu_pipe_pname,
						&f->fu_pipe_pid)) < 0) {
				f->f_type = F_UNUSED;
				logerror(f->fu_pipe_pname);
				break;
			}
		}
		if (writev(f->f_file, iov, IOV_SIZE) < 0) {
			int e = errno;
			close_filed(f);
			if (f->fu_pipe_pid > 0)
				deadq_enter(f->fu_pipe_pid,
					    f->fu_pipe_pname);
			f->fu_pipe_pid = 0;
			errno = e;
			logerror(f->fu_pipe_pname);
		}
		break;

	case F_CONSOLE:
		if (flags & IGN_CONS) {
			dprintf(" (ignored)\n");
			break;
		}
		/* FALLTHROUGH */

	case F_TTY:
		dprintf(" %s%s\n", _PATH_DEV, f->fu_fname);
		v->iov_base = crlf;
		v->iov_len = 2;

		errno = 0;	/* ttymsg() only sometimes returns an errno */
		if ((msgret = ttymsg(iov, IOV_SIZE, f->fu_fname, 10))) {
			f->f_type = F_UNUSED;
			logerror(msgret);
		}
		break;

	case F_USERS:
	case F_WALL:
		dprintf("\n");
		v->iov_base = crlf;
		v->iov_len = 2;
		wallmsg(f, iov, IOV_SIZE);
		break;
	}
	f->f_prevcount = 0;
	free(wmsg);
}

/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */
static void
wallmsg(struct filed *f, struct iovec *iov, const int iovlen)
{
	static int reenter;			/* avoid calling ourselves */
	struct utmpx *ut;
	int i;
	const char *p;

	if (reenter++)
		return;
	setutxent();
	/* NOSTRICT */
	while ((ut = getutxent()) != NULL) {
		if (ut->ut_type != USER_PROCESS)
			continue;
		if (f->f_type == F_WALL) {
			if ((p = ttymsg(iov, iovlen, ut->ut_line,
			    TTYMSGTIME)) != NULL) {
				errno = 0;	/* already in msg */
				logerror(p);
			}
			continue;
		}
		/* should we send the message to this user? */
		for (i = 0; i < MAXUNAMES; i++) {
			if (!f->fu_uname[i][0])
				break;
			if (!strcmp(f->fu_uname[i], ut->ut_user)) {
				if ((p = ttymsg_check(iov, iovlen, ut->ut_line,
				    TTYMSGTIME)) != NULL) {
					errno = 0;	/* already in msg */
					logerror(p);
				}
				break;
			}
		}
	}
	endutxent();
	reenter = 0;
}

/*
 * Wrapper routine for ttymsg() that checks the terminal for messages enabled.
 */
static const char *
ttymsg_check(struct iovec *iov, int iovcnt, char *line, int tmout)
{
	static char device[1024];
	static char errbuf[1024];
	struct stat sb;

	(void) snprintf(device, sizeof(device), "%s%s", _PATH_DEV, line);

	if (stat(device, &sb) < 0) {
		(void) snprintf(errbuf, sizeof(errbuf),
		    "%s: %s", device, strerror(errno));
		return (errbuf);
	}
	if ((sb.st_mode & S_IWGRP) == 0)
		/* Messages disabled. */
		return (NULL);
	return ttymsg(iov, iovcnt, line, tmout);
}

static void
reapchild(int signo __unused)
{
	int status;
	pid_t pid;
	struct filed *f;

	while ((pid = wait3(&status, WNOHANG, (struct rusage *)NULL)) > 0) {
		if (!Initialized)
			/* Don't tell while we are initting. */
			continue;

		/* First, look if it's a process from the dead queue. */
		if (deadq_remove(pid))
			continue;

		/* Now, look in list of active processes. */
		STAILQ_FOREACH(f, &fhead, next) {
			if (f->f_type == F_PIPE &&
			    f->fu_pipe_pid == pid) {
				close_filed(f);
				f->fu_pipe_pid = 0;
				log_deadchild(pid, status, f->fu_pipe_pname);
				break;
			}
		}
	}
}

/*
 * Return a printable representation of a host address.
 */
static const char *
cvthname(struct sockaddr *f)
{
	int error, hl;
	sigset_t omask, nmask;
	static char hname[NI_MAXHOST], ip[NI_MAXHOST];

	dprintf("cvthname(%d) len = %d, %zu\n", f->sa_family, f->sa_len, sizeof(struct sockaddr_in6));
	error = getnameinfo(f, f->sa_len, ip, sizeof(ip), NULL, 0,
		    NI_NUMERICHOST);
	dprintf("cvthname(%s)\n", ip);

	if (error) {
		dprintf("Malformed from address %s\n", gai_strerror(error));
		return ("???");
	}
	if (!resolve)
		return (ip);

	sigemptyset(&nmask);
	sigaddset(&nmask, SIGHUP);
	sigprocmask(SIG_BLOCK, &nmask, &omask);
	error = getnameinfo((struct sockaddr *)f,
			    ((struct sockaddr *)f)->sa_len,
			    hname, sizeof hname, NULL, 0, NI_NAMEREQD);
	sigprocmask(SIG_SETMASK, &omask, NULL);
	if (error) {
		dprintf("Host name for your address (%s) unknown\n", ip);
		return (ip);
	}
	hl = strlen(hname);
	if (hl > 0 && hname[hl-1] == '.')
		hname[--hl] = '\0';
	trimdomain(hname, hl);
	return (hname);
}

static void
dodie(int signo)
{

	WantDie = signo;
}

static void
domark(int signo __unused)
{

	MarkSet = 1;
}

/*
 * Print syslogd errors some place.
 */
static void
logerror(const char *type)
{
	char buf[512];
	static int recursed = 0;

	/* If there's an error while trying to log an error, give up. */
	if (recursed)
		return;
	recursed++;
	if (errno)
		(void)snprintf(buf,
		    sizeof buf, "syslogd: %s: %s", type, strerror(errno));
	else
		(void)snprintf(buf, sizeof buf, "syslogd: %s", type);
	errno = 0;
	dprintf("%s\n", buf);
	logmsg(LOG_SYSLOG|LOG_ERR, buf, LocalHostName, ADDDATE);
	recursed--;
}

static void
die(int signo)
{
	struct filed *f;
	struct socklist *sl;
	int was_initialized;
	char buf[100];

	was_initialized = Initialized;
	Initialized = 0;	/* Don't log SIGCHLDs. */
	STAILQ_FOREACH(f, &fhead, next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);
		if (f->f_type == F_PIPE && f->fu_pipe_pid > 0) {
			close_filed(f);
			f->fu_pipe_pid = 0;
		}
	}
	Initialized = was_initialized;
	if (signo) {
		dprintf("syslogd: exiting on signal %d\n", signo);
		(void)snprintf(buf, sizeof(buf), "exiting on signal %d", signo);
		errno = 0;
		logerror(buf);
	}
	STAILQ_FOREACH(sl, &shead, next) {
		if (sl->sl_ss.ss_family == AF_LOCAL)
			unlink(sl->sl_peer->pe_name);
	}
	pidfile_remove(pfh);

	exit(1);
}

static int
configfiles(const struct dirent *dp)
{
	const char *p;
	size_t ext_len;

	if (dp->d_name[0] == '.')
		return (0);

	ext_len = sizeof(include_ext) -1;

	if (dp->d_namlen <= ext_len)
		return (0);

	p = &dp->d_name[dp->d_namlen - ext_len];
	if (strcmp(p, include_ext) != 0)
		return (0);

	return (1);
}

static void
readconfigfile(FILE *cf, int allow_includes)
{
	FILE *cf2;
	struct filed *f;
	struct dirent **ent;
	char cline[LINE_MAX];
	char host[MAXHOSTNAMELEN];
	char prog[LINE_MAX];
	char file[MAXPATHLEN];
	char *p, *tmp;
	int i, nents;
	size_t include_len;

	/*
	 *  Foreach line in the conf table, open that file.
	 */
	f = NULL;
	include_len = sizeof(include_str) -1;
	(void)strlcpy(host, "*", sizeof(host));
	(void)strlcpy(prog, "*", sizeof(prog));
	while (fgets(cline, sizeof(cline), cf) != NULL) {
		/*
		 * check for end-of-section, comments, strip off trailing
		 * spaces and newline character. #!prog is treated specially:
		 * following lines apply only to that program.
		 */
		for (p = cline; isspace(*p); ++p)
			continue;
		if (*p == 0)
			continue;
		if (allow_includes &&
		    strncmp(p, include_str, include_len) == 0 &&
		    isspace(p[include_len])) {
			p += include_len;
			while (isspace(*p))
				p++;
			tmp = p;
			while (*tmp != '\0' && !isspace(*tmp))
				tmp++;
			*tmp = '\0';
			dprintf("Trying to include files in '%s'\n", p);
			nents = scandir(p, &ent, configfiles, alphasort);
			if (nents == -1) {
				dprintf("Unable to open '%s': %s\n", p,
				    strerror(errno));
				continue;
			}
			for (i = 0; i < nents; i++) {
				if (snprintf(file, sizeof(file), "%s/%s", p,
				    ent[i]->d_name) >= (int)sizeof(file)) {
					dprintf("ignoring path too long: "
					    "'%s/%s'\n", p, ent[i]->d_name);
					free(ent[i]);
					continue;
				}
				free(ent[i]);
				cf2 = fopen(file, "r");
				if (cf2 == NULL)
					continue;
				dprintf("reading %s\n", file);
				readconfigfile(cf2, 0);
				fclose(cf2);
			}
			free(ent);
			continue;
		}
		if (*p == '#') {
			p++;
			if (*p != '!' && *p != '+' && *p != '-')
				continue;
		}
		if (*p == '+' || *p == '-') {
			host[0] = *p++;
			while (isspace(*p))
				p++;
			if ((!*p) || (*p == '*')) {
				(void)strlcpy(host, "*", sizeof(host));
				continue;
			}
			if (*p == '@')
				p = LocalHostName;
			for (i = 1; i < MAXHOSTNAMELEN - 1; i++) {
				if (!isalnum(*p) && *p != '.' && *p != '-'
				    && *p != ',' && *p != ':' && *p != '%')
					break;
				host[i] = *p++;
			}
			host[i] = '\0';
			continue;
		}
		if (*p == '!') {
			p++;
			while (isspace(*p)) p++;
			if ((!*p) || (*p == '*')) {
				(void)strlcpy(prog, "*", sizeof(prog));
				continue;
			}
			for (i = 0; i < LINE_MAX - 1; i++) {
				if (!isprint(p[i]) || isspace(p[i]))
					break;
				prog[i] = p[i];
			}
			prog[i] = 0;
			continue;
		}
		for (p = cline + 1; *p != '\0'; p++) {
			if (*p != '#')
				continue;
			if (*(p - 1) == '\\') {
				strcpy(p - 1, p);
				p--;
				continue;
			}
			*p = '\0';
			break;
		}
		for (i = strlen(cline) - 1; i >= 0 && isspace(cline[i]); i--)
			cline[i] = '\0';
		f = (struct filed *)calloc(1, sizeof(*f));
		if (f == NULL) {
			logerror("calloc");
			exit(1);
		}
		STAILQ_INSERT_TAIL(&fhead, f, next);
		cfline(cline, f, prog, host);
	}
}

/*
 *  INIT -- Initialize syslogd from configuration table
 */
static void
init(int signo)
{
	int i;
	FILE *cf;
	struct filed *f;
	char *p;
	char oldLocalHostName[MAXHOSTNAMELEN];
	char hostMsg[2*MAXHOSTNAMELEN+40];
	char bootfileMsg[LINE_MAX];

	dprintf("init\n");

	/*
	 * Load hostname (may have changed).
	 */
	if (signo != 0)
		(void)strlcpy(oldLocalHostName, LocalHostName,
		    sizeof(oldLocalHostName));
	if (gethostname(LocalHostName, sizeof(LocalHostName)))
		err(EX_OSERR, "gethostname() failed");
	if ((p = strchr(LocalHostName, '.')) != NULL) {
		*p++ = '\0';
		LocalDomain = p;
	} else {
		LocalDomain = "";
	}

	/*
	 * Load / reload timezone data (in case it changed).
	 *
	 * Just calling tzset() again does not work, the timezone code
	 * caches the result.  However, by setting the TZ variable, one
	 * can defeat the caching and have the timezone code really
	 * reload the timezone data.  Respect any initial setting of
	 * TZ, in case the system is configured specially.
	 */
	dprintf("loading timezone data via tzset()\n");
	if (getenv("TZ")) {
		tzset();
	} else {
		setenv("TZ", ":/etc/localtime", 1);
		tzset();
		unsetenv("TZ");
	}

	/*
	 *  Close all open log files.
	 */
	Initialized = 0;
	STAILQ_FOREACH(f, &fhead, next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);

		switch (f->f_type) {
		case F_FILE:
		case F_FORW:
		case F_CONSOLE:
		case F_TTY:
			close_filed(f);
			break;
		case F_PIPE:
			if (f->fu_pipe_pid > 0) {
				close_filed(f);
				deadq_enter(f->fu_pipe_pid,
					    f->fu_pipe_pname);
			}
			f->fu_pipe_pid = 0;
			break;
		}
	}
	while(!STAILQ_EMPTY(&fhead)) {
		f = STAILQ_FIRST(&fhead);
		STAILQ_REMOVE_HEAD(&fhead, next);
		free(f->f_program);
		free(f->f_host);
		free(f);
	}

	/* open the configuration file */
	if ((cf = fopen(ConfFile, "r")) == NULL) {
		dprintf("cannot open %s\n", ConfFile);
		f = calloc(1, sizeof(*f));
		if (f == NULL) {
			logerror("calloc");
			exit(1);
		}
		cfline("*.ERR\t/dev/console", f, "*", "*");
		STAILQ_INSERT_TAIL(&fhead, f, next);

		f = calloc(1, sizeof(*f));
		if (f == NULL) {
			logerror("calloc");
			exit(1);
		}
		cfline("*.PANIC\t*", f, "*", "*");
		STAILQ_INSERT_TAIL(&fhead, f, next);

		Initialized = 1;
		return;
	}

	readconfigfile(cf, 1);

	/* close the configuration file */
	(void)fclose(cf);

	Initialized = 1;

	if (Debug) {
		int port;
		STAILQ_FOREACH(f, &fhead, next) {
			for (i = 0; i <= LOG_NFACILITIES; i++)
				if (f->f_pmask[i] == INTERNAL_NOPRI)
					printf("X ");
				else
					printf("%d ", f->f_pmask[i]);
			printf("%s: ", TypeNames[f->f_type]);
			switch (f->f_type) {
			case F_FILE:
				printf("%s", f->fu_fname);
				break;

			case F_CONSOLE:
			case F_TTY:
				printf("%s%s", _PATH_DEV, f->fu_fname);
				break;

			case F_FORW:
				port = ntohs(satosin(f->fu_forw_addr->ai_addr)->sin_port);
				if (port != 514) {
					printf("%s:%d",
						f->fu_forw_hname, port);
				} else {
					printf("%s", f->fu_forw_hname);
				}
				break;

			case F_PIPE:
				printf("%s", f->fu_pipe_pname);
				break;

			case F_USERS:
				for (i = 0; i < MAXUNAMES && *f->fu_uname[i]; i++)
					printf("%s, ", f->fu_uname[i]);
				break;
			}
			if (f->f_program)
				printf(" (%s)", f->f_program);
			printf("\n");
		}
	}

	logmsg(LOG_SYSLOG|LOG_INFO, "syslogd: restart", LocalHostName, ADDDATE);
	dprintf("syslogd: restarted\n");
	/*
	 * Log a change in hostname, but only on a restart.
	 */
	if (signo != 0 && strcmp(oldLocalHostName, LocalHostName) != 0) {
		(void)snprintf(hostMsg, sizeof(hostMsg),
		    "syslogd: hostname changed, \"%s\" to \"%s\"",
		    oldLocalHostName, LocalHostName);
		logmsg(LOG_SYSLOG|LOG_INFO, hostMsg, LocalHostName, ADDDATE);
		dprintf("%s\n", hostMsg);
	}
	/*
	 * Log the kernel boot file if we aren't going to use it as
	 * the prefix, and if this is *not* a restart.
	 */
	if (signo == 0 && !use_bootfile) {
		(void)snprintf(bootfileMsg, sizeof(bootfileMsg),
		    "syslogd: kernel boot file is %s", bootfile);
		logmsg(LOG_KERN|LOG_INFO, bootfileMsg, LocalHostName, ADDDATE);
		dprintf("%s\n", bootfileMsg);
	}
}

/*
 * Crack a configuration file line
 */
static void
cfline(const char *line, struct filed *f, const char *prog, const char *host)
{
	struct addrinfo hints, *res;
	int error, i, pri, syncfile;
	const char *p, *q;
	char *bp;
	char buf[MAXLINE], ebuf[100];

	dprintf("cfline(\"%s\", f, \"%s\", \"%s\")\n", line, prog, host);

	errno = 0;	/* keep strerror() stuff out of logerror messages */

	/* clear out file entry */
	memset(f, 0, sizeof(*f));
	for (i = 0; i <= LOG_NFACILITIES; i++)
		f->f_pmask[i] = INTERNAL_NOPRI;

	/* save hostname if any */
	if (host && *host == '*')
		host = NULL;
	if (host) {
		int hl;

		f->f_host = strdup(host);
		if (f->f_host == NULL) {
			logerror("strdup");
			exit(1);
		}
		hl = strlen(f->f_host);
		if (hl > 0 && f->f_host[hl-1] == '.')
			f->f_host[--hl] = '\0';
		trimdomain(f->f_host, hl);
	}

	/* save program name if any */
	if (prog && *prog == '*')
		prog = NULL;
	if (prog) {
		f->f_program = strdup(prog);
		if (f->f_program == NULL) {
			logerror("strdup");
			exit(1);
		}
	}

	/* scan through the list of selectors */
	for (p = line; *p && *p != '\t' && *p != ' ';) {
		int pri_done;
		int pri_cmp;
		int pri_invert;

		/* find the end of this facility name list */
		for (q = p; *q && *q != '\t' && *q != ' ' && *q++ != '.'; )
			continue;

		/* get the priority comparison */
		pri_cmp = 0;
		pri_done = 0;
		pri_invert = 0;
		if (*q == '!') {
			pri_invert = 1;
			q++;
		}
		while (!pri_done) {
			switch (*q) {
			case '<':
				pri_cmp |= PRI_LT;
				q++;
				break;
			case '=':
				pri_cmp |= PRI_EQ;
				q++;
				break;
			case '>':
				pri_cmp |= PRI_GT;
				q++;
				break;
			default:
				pri_done++;
				break;
			}
		}

		/* collect priority name */
		for (bp = buf; *q && !strchr("\t,; ", *q); )
			*bp++ = *q++;
		*bp = '\0';

		/* skip cruft */
		while (strchr(",;", *q))
			q++;

		/* decode priority name */
		if (*buf == '*') {
			pri = LOG_PRIMASK;
			pri_cmp = PRI_LT | PRI_EQ | PRI_GT;
		} else {
			/* Ignore trailing spaces. */
			for (i = strlen(buf) - 1; i >= 0 && buf[i] == ' '; i--)
				buf[i] = '\0';

			pri = decode(buf, prioritynames);
			if (pri < 0) {
				errno = 0;
				(void)snprintf(ebuf, sizeof ebuf,
				    "unknown priority name \"%s\"", buf);
				logerror(ebuf);
				return;
			}
		}
		if (!pri_cmp)
			pri_cmp = (UniquePriority)
				  ? (PRI_EQ)
				  : (PRI_EQ | PRI_GT)
				  ;
		if (pri_invert)
			pri_cmp ^= PRI_LT | PRI_EQ | PRI_GT;

		/* scan facilities */
		while (*p && !strchr("\t.; ", *p)) {
			for (bp = buf; *p && !strchr("\t,;. ", *p); )
				*bp++ = *p++;
			*bp = '\0';

			if (*buf == '*') {
				for (i = 0; i < LOG_NFACILITIES; i++) {
					f->f_pmask[i] = pri;
					f->f_pcmp[i] = pri_cmp;
				}
			} else {
				i = decode(buf, facilitynames);
				if (i < 0) {
					errno = 0;
					(void)snprintf(ebuf, sizeof ebuf,
					    "unknown facility name \"%s\"",
					    buf);
					logerror(ebuf);
					return;
				}
				f->f_pmask[i >> 3] = pri;
				f->f_pcmp[i >> 3] = pri_cmp;
			}
			while (*p == ',' || *p == ' ')
				p++;
		}

		p = q;
	}

	/* skip to action part */
	while (*p == '\t' || *p == ' ')
		p++;

	if (*p == '-') {
		syncfile = 0;
		p++;
	} else
		syncfile = 1;

	switch (*p) {
	case '@':
		{
			char *tp;
			char endkey = ':';
			/*
			 * scan forward to see if there is a port defined.
			 * so we can't use strlcpy..
			 */
			i = sizeof(f->fu_forw_hname);
			tp = f->fu_forw_hname;
			p++;

			/*
			 * an ipv6 address should start with a '[' in that case
			 * we should scan for a ']'
			 */
			if (*p == '[') {
				p++;
				endkey = ']';
			}
			while (*p && (*p != endkey) && (i-- > 0)) {
				*tp++ = *p++;
			}
			if (endkey == ']' && *p == endkey)
				p++;
			*tp = '\0';
		}
		/* See if we copied a domain and have a port */
		if (*p == ':')
			p++;
		else
			p = NULL;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = SOCK_DGRAM;
		error = getaddrinfo(f->fu_forw_hname,
				p ? p : "syslog", &hints, &res);
		if (error) {
			logerror(gai_strerror(error));
			break;
		}
		f->fu_forw_addr = res;
		f->f_type = F_FORW;
		break;

	case '/':
		if ((f->f_file = open(p, logflags, 0600)) < 0) {
			f->f_type = F_UNUSED;
			logerror(p);
			break;
		}
		if (syncfile)
			f->f_flags |= FFLAG_SYNC;
		if (isatty(f->f_file)) {
			if (strcmp(p, ctty) == 0)
				f->f_type = F_CONSOLE;
			else
				f->f_type = F_TTY;
			(void)strlcpy(f->fu_fname, p + sizeof(_PATH_DEV) - 1,
			    sizeof(f->fu_fname));
		} else {
			(void)strlcpy(f->fu_fname, p, sizeof(f->fu_fname));
			f->f_type = F_FILE;
		}
		break;

	case '|':
		f->fu_pipe_pid = 0;
		(void)strlcpy(f->fu_pipe_pname, p + 1,
		    sizeof(f->fu_pipe_pname));
		f->f_type = F_PIPE;
		break;

	case '*':
		f->f_type = F_WALL;
		break;

	default:
		for (i = 0; i < MAXUNAMES && *p; i++) {
			for (q = p; *q && *q != ','; )
				q++;
			(void)strncpy(f->fu_uname[i], p, MAXLOGNAME - 1);
			if ((q - p) >= MAXLOGNAME)
				f->fu_uname[i][MAXLOGNAME - 1] = '\0';
			else
				f->fu_uname[i][q - p] = '\0';
			while (*q == ',' || *q == ' ')
				q++;
			p = q;
		}
		f->f_type = F_USERS;
		break;
	}
}


/*
 *  Decode a symbolic name to a numeric value
 */
static int
decode(const char *name, const CODE *codetab)
{
	const CODE *c;
	char *p, buf[40];

	if (isdigit(*name))
		return (atoi(name));

	for (p = buf; *name && p < &buf[sizeof(buf) - 1]; p++, name++) {
		if (isupper(*name))
			*p = tolower(*name);
		else
			*p = *name;
	}
	*p = '\0';
	for (c = codetab; c->c_name; c++)
		if (!strcmp(buf, c->c_name))
			return (c->c_val);

	return (-1);
}

static void
markit(void)
{
	struct filed *f;
	dq_t q, next;

	now = time((time_t *)NULL);
	MarkSeq += TIMERINTVL;
	if (MarkSeq >= MarkInterval) {
		logmsg(LOG_INFO, "-- MARK --",
		    LocalHostName, ADDDATE|MARK);
		MarkSeq = 0;
	}

	STAILQ_FOREACH(f, &fhead, next) {
		if (f->f_prevcount && now >= REPEATTIME(f)) {
			dprintf("flush %s: repeated %d times, %d sec.\n",
			    TypeNames[f->f_type], f->f_prevcount,
			    repeatinterval[f->f_repeatcount]);
			fprintlog(f, 0, (char *)NULL);
			BACKOFF(f);
		}
	}

	/* Walk the dead queue, and see if we should signal somebody. */
	for (q = TAILQ_FIRST(&deadq_head); q != NULL; q = next) {
		next = TAILQ_NEXT(q, dq_entries);

		switch (q->dq_timeout) {
		case 0:
			/* Already signalled once, try harder now. */
			if (kill(q->dq_pid, SIGKILL) != 0)
				(void)deadq_remove(q->dq_pid);
			break;

		case 1:
			/*
			 * Timed out on dead queue, send terminate
			 * signal.  Note that we leave the removal
			 * from the dead queue to reapchild(), which
			 * will also log the event (unless the process
			 * didn't even really exist, in case we simply
			 * drop it from the dead queue).
			 */
			if (kill(q->dq_pid, SIGTERM) != 0)
				(void)deadq_remove(q->dq_pid);
			/* FALLTHROUGH */

		default:
			q->dq_timeout--;
		}
	}
	MarkSet = 0;
	(void)alarm(TIMERINTVL);
}

/*
 * fork off and become a daemon, but wait for the child to come online
 * before returing to the parent, or we get disk thrashing at boot etc.
 * Set a timer so we don't hang forever if it wedges.
 */
static int
waitdaemon(int nochdir, int noclose, int maxwait)
{
	int fd;
	int status;
	pid_t pid, childpid;

	switch (childpid = fork()) {
	case -1:
		return (-1);
	case 0:
		break;
	default:
		signal(SIGALRM, timedout);
		alarm(maxwait);
		while ((pid = wait3(&status, 0, NULL)) != -1) {
			if (WIFEXITED(status))
				errx(1, "child pid %d exited with return code %d",
					pid, WEXITSTATUS(status));
			if (WIFSIGNALED(status))
				errx(1, "child pid %d exited on signal %d%s",
					pid, WTERMSIG(status),
					WCOREDUMP(status) ? " (core dumped)" :
					"");
			if (pid == childpid)	/* it's gone... */
				break;
		}
		exit(0);
	}

	if (setsid() == -1)
		return (-1);

	if (!nochdir)
		(void)chdir("/");

	if (!noclose && (fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		(void)dup2(fd, STDIN_FILENO);
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > 2)
			(void)close (fd);
	}
	return (getppid());
}

/*
 * We get a SIGALRM from the child when it's running and finished doing it's
 * fsync()'s or O_SYNC writes for all the boot messages.
 *
 * We also get a signal from the kernel if the timer expires, so check to
 * see what happened.
 */
static void
timedout(int sig __unused)
{
	int left;
	left = alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (left == 0)
		errx(1, "timed out waiting for child");
	else
		_exit(0);
}

/*
 * Add `s' to the list of allowable peer addresses to accept messages
 * from.
 *
 * `s' is a string in the form:
 *
 *    [*]domainname[:{servicename|portnumber|*}]
 *
 * or
 *
 *    netaddr/maskbits[:{servicename|portnumber|*}]
 *
 * Returns -1 on error, 0 if the argument was valid.
 */
static int
allowaddr(char *s)
{
	char *cp1, *cp2;
	struct allowedpeer *ap;
	struct servent *se;
	int masklen = -1;
	struct addrinfo hints, *res;
	struct in_addr *addrp, *maskp;
#ifdef INET6
	int i;
	u_int32_t *addr6p, *mask6p;
#endif
	char ip[NI_MAXHOST];

	ap = calloc(1, sizeof(*ap));
	if (ap == NULL)
		err(1, "malloc failed");

#ifdef INET6
	if (*s != '[' || (cp1 = strchr(s + 1, ']')) == NULL)
#endif
		cp1 = s;
	if ((cp1 = strrchr(cp1, ':'))) {
		/* service/port provided */
		*cp1++ = '\0';
		if (strlen(cp1) == 1 && *cp1 == '*')
			/* any port allowed */
			ap->port = 0;
		else if ((se = getservbyname(cp1, "udp"))) {
			ap->port = ntohs(se->s_port);
		} else {
			ap->port = strtol(cp1, &cp2, 0);
			if (*cp2 != '\0')
				return (-1); /* port not numeric */
		}
	} else {
		if ((se = getservbyname("syslog", "udp")))
			ap->port = ntohs(se->s_port);
		else
			/* sanity, should not happen */
			ap->port = 514;
	}

	if ((cp1 = strchr(s, '/')) != NULL &&
	    strspn(cp1 + 1, "0123456789") == strlen(cp1 + 1)) {
		*cp1 = '\0';
		if ((masklen = atoi(cp1 + 1)) < 0)
			return (-1);
	}
#ifdef INET6
	if (*s == '[') {
		cp2 = s + strlen(s) - 1;
		if (*cp2 == ']') {
			++s;
			*cp2 = '\0';
		} else {
			cp2 = NULL;
		}
	} else {
		cp2 = NULL;
	}
#endif
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
	if (getaddrinfo(s, NULL, &hints, &res) == 0) {
		ap->isnumeric = 1;
		memcpy(&ap->a_addr, res->ai_addr, res->ai_addrlen);
		memset(&ap->a_mask, 0, sizeof(ap->a_mask));
		ap->a_mask.ss_family = res->ai_family;
		if (res->ai_family == AF_INET) {
			ap->a_mask.ss_len = sizeof(struct sockaddr_in);
			maskp = &((struct sockaddr_in *)&ap->a_mask)->sin_addr;
			addrp = &((struct sockaddr_in *)&ap->a_addr)->sin_addr;
			if (masklen < 0) {
				/* use default netmask */
				if (IN_CLASSA(ntohl(addrp->s_addr)))
					maskp->s_addr = htonl(IN_CLASSA_NET);
				else if (IN_CLASSB(ntohl(addrp->s_addr)))
					maskp->s_addr = htonl(IN_CLASSB_NET);
				else
					maskp->s_addr = htonl(IN_CLASSC_NET);
			} else if (masklen <= 32) {
				/* convert masklen to netmask */
				if (masklen == 0)
					maskp->s_addr = 0;
				else
					maskp->s_addr = htonl(~((1 << (32 - masklen)) - 1));
			} else {
				freeaddrinfo(res);
				return (-1);
			}
			/* Lose any host bits in the network number. */
			addrp->s_addr &= maskp->s_addr;
		}
#ifdef INET6
		else if (res->ai_family == AF_INET6 && masklen <= 128) {
			ap->a_mask.ss_len = sizeof(struct sockaddr_in6);
			if (masklen < 0)
				masklen = 128;
			mask6p = (u_int32_t *)&((struct sockaddr_in6 *)&ap->a_mask)->sin6_addr;
			/* convert masklen to netmask */
			while (masklen > 0) {
				if (masklen < 32) {
					*mask6p = htonl(~(0xffffffff >> masklen));
					break;
				}
				*mask6p++ = 0xffffffff;
				masklen -= 32;
			}
			/* Lose any host bits in the network number. */
			mask6p = (u_int32_t *)&((struct sockaddr_in6 *)&ap->a_mask)->sin6_addr;
			addr6p = (u_int32_t *)&((struct sockaddr_in6 *)&ap->a_addr)->sin6_addr;
			for (i = 0; i < 4; i++)
				addr6p[i] &= mask6p[i];
		}
#endif
		else {
			freeaddrinfo(res);
			return (-1);
		}
		freeaddrinfo(res);
	} else {
		/* arg `s' is domain name */
		ap->isnumeric = 0;
		ap->a_name = s;
		if (cp1)
			*cp1 = '/';
#ifdef INET6
		if (cp2) {
			*cp2 = ']';
			--s;
		}
#endif
	}
	STAILQ_INSERT_TAIL(&aphead, ap, next);

	if (Debug) {
		printf("allowaddr: rule ");
		if (ap->isnumeric) {
			printf("numeric, ");
			getnameinfo((struct sockaddr *)&ap->a_addr,
				    ((struct sockaddr *)&ap->a_addr)->sa_len,
				    ip, sizeof ip, NULL, 0, NI_NUMERICHOST);
			printf("addr = %s, ", ip);
			getnameinfo((struct sockaddr *)&ap->a_mask,
				    ((struct sockaddr *)&ap->a_mask)->sa_len,
				    ip, sizeof ip, NULL, 0, NI_NUMERICHOST);
			printf("mask = %s; ", ip);
		} else {
			printf("domainname = %s; ", ap->a_name);
		}
		printf("port = %d\n", ap->port);
	}
	return (0);
}

/*
 * Validate that the remote peer has permission to log to us.
 */
static int
validate(struct sockaddr *sa, const char *hname)
{
	int i;
	size_t l1, l2;
	char *cp, name[NI_MAXHOST], ip[NI_MAXHOST], port[NI_MAXSERV];
	struct allowedpeer *ap;
	struct sockaddr_in *sin4, *a4p = NULL, *m4p = NULL;
#ifdef INET6
	struct sockaddr_in6 *sin6, *a6p = NULL, *m6p = NULL;
#endif
	struct addrinfo hints, *res;
	u_short sport;
	int num = 0;

	STAILQ_FOREACH(ap, &aphead, next) {
		num++;
	}
	dprintf("# of validation rule: %d\n", num);
	if (num == 0)
		/* traditional behaviour, allow everything */
		return (1);

	(void)strlcpy(name, hname, sizeof(name));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
	if (getaddrinfo(name, NULL, &hints, &res) == 0)
		freeaddrinfo(res);
	else if (strchr(name, '.') == NULL) {
		strlcat(name, ".", sizeof name);
		strlcat(name, LocalDomain, sizeof name);
	}
	if (getnameinfo(sa, sa->sa_len, ip, sizeof(ip), port, sizeof(port),
			NI_NUMERICHOST | NI_NUMERICSERV) != 0)
		return (0);	/* for safety, should not occur */
	dprintf("validate: dgram from IP %s, port %s, name %s;\n",
		ip, port, name);
	sport = atoi(port);

	/* now, walk down the list */
	i = 0;
	STAILQ_FOREACH(ap, &aphead, next) {
		i++;
		if (ap->port != 0 && ap->port != sport) {
			dprintf("rejected in rule %d due to port mismatch.\n",
			    i);
			continue;
		}

		if (ap->isnumeric) {
			if (ap->a_addr.ss_family != sa->sa_family) {
				dprintf("rejected in rule %d due to address family mismatch.\n", i);
				continue;
			}
			if (ap->a_addr.ss_family == AF_INET) {
				sin4 = satosin(sa);
				a4p = satosin(&ap->a_addr);
				m4p = satosin(&ap->a_mask);
				if ((sin4->sin_addr.s_addr & m4p->sin_addr.s_addr)
				    != a4p->sin_addr.s_addr) {
					dprintf("rejected in rule %d due to IP mismatch.\n", i);
					continue;
				}
			}
#ifdef INET6
			else if (ap->a_addr.ss_family == AF_INET6) {
				sin6 = satosin6(sa);
				a6p = satosin6(&ap->a_addr);
				m6p = satosin6(&ap->a_mask);
				if (a6p->sin6_scope_id != 0 &&
				    sin6->sin6_scope_id != a6p->sin6_scope_id) {
					dprintf("rejected in rule %d due to scope mismatch.\n", i);
					continue;
				}
				if (IN6_ARE_MASKED_ADDR_EQUAL(&sin6->sin6_addr,
				    &a6p->sin6_addr, &m6p->sin6_addr) != 0) {
					dprintf("rejected in rule %d due to IP mismatch.\n", i);
					continue;
				}
			}
#endif
			else
				continue;
		} else {
			cp = ap->a_name;
			l1 = strlen(name);
			if (*cp == '*') {
				/* allow wildmatch */
				cp++;
				l2 = strlen(cp);
				if (l2 > l1 || memcmp(cp, &name[l1 - l2], l2) != 0) {
					dprintf("rejected in rule %d due to name mismatch.\n", i);
					continue;
				}
			} else {
				/* exact match */
				l2 = strlen(cp);
				if (l2 != l1 || memcmp(cp, name, l1) != 0) {
					dprintf("rejected in rule %d due to name mismatch.\n", i);
					continue;
				}
			}
		}
		dprintf("accepted in rule %d.\n", i);
		return (1);	/* hooray! */
	}
	return (0);
}

/*
 * Fairly similar to popen(3), but returns an open descriptor, as
 * opposed to a FILE *.
 */
static int
p_open(const char *prog, pid_t *rpid)
{
	int pfd[2], nulldesc;
	pid_t pid;
	sigset_t omask, mask;
	char *argv[4]; /* sh -c cmd NULL */
	char errmsg[200];

	if (pipe(pfd) == -1)
		return (-1);
	if ((nulldesc = open(_PATH_DEVNULL, O_RDWR)) == -1)
		/* we are royally screwed anyway */
		return (-1);

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigaddset(&mask, SIGHUP);
	sigprocmask(SIG_BLOCK, &mask, &omask);
	switch ((pid = fork())) {
	case -1:
		sigprocmask(SIG_SETMASK, &omask, 0);
		close(nulldesc);
		return (-1);

	case 0:
		argv[0] = strdup("sh");
		argv[1] = strdup("-c");
		argv[2] = strdup(prog);
		argv[3] = NULL;
		if (argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) {
			logerror("strdup");
			exit(1);
		}

		alarm(0);
		(void)setsid();	/* Avoid catching SIGHUPs. */

		/*
		 * Throw away pending signals, and reset signal
		 * behaviour to standard values.
		 */
		signal(SIGALRM, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		sigprocmask(SIG_SETMASK, &omask, 0);
		signal(SIGPIPE, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGALRM, SIG_DFL);
		signal(SIGHUP, SIG_DFL);

		dup2(pfd[0], STDIN_FILENO);
		dup2(nulldesc, STDOUT_FILENO);
		dup2(nulldesc, STDERR_FILENO);
		closefrom(3);

		(void)execvp(_PATH_BSHELL, argv);
		_exit(255);
	}

	sigprocmask(SIG_SETMASK, &omask, 0);
	close(nulldesc);
	close(pfd[0]);
	/*
	 * Avoid blocking on a hung pipe.  With O_NONBLOCK, we are
	 * supposed to get an EWOULDBLOCK on writev(2), which is
	 * caught by the logic above anyway, which will in turn close
	 * the pipe, and fork a new logging subprocess if necessary.
	 * The stale subprocess will be killed some time later unless
	 * it terminated itself due to closing its input pipe (so we
	 * get rid of really dead puppies).
	 */
	if (fcntl(pfd[1], F_SETFL, O_NONBLOCK) == -1) {
		/* This is bad. */
		(void)snprintf(errmsg, sizeof errmsg,
			       "Warning: cannot change pipe to PID %d to "
			       "non-blocking behaviour.",
			       (int)pid);
		logerror(errmsg);
	}
	*rpid = pid;
	return (pfd[1]);
}

static void
deadq_enter(pid_t pid, const char *name)
{
	dq_t p;
	int status;

	/*
	 * Be paranoid, if we can't signal the process, don't enter it
	 * into the dead queue (perhaps it's already dead).  If possible,
	 * we try to fetch and log the child's status.
	 */
	if (kill(pid, 0) != 0) {
		if (waitpid(pid, &status, WNOHANG) > 0)
			log_deadchild(pid, status, name);
		return;
	}

	p = malloc(sizeof(struct deadq_entry));
	if (p == NULL) {
		logerror("malloc");
		exit(1);
	}

	p->dq_pid = pid;
	p->dq_timeout = DQ_TIMO_INIT;
	TAILQ_INSERT_TAIL(&deadq_head, p, dq_entries);
}

static int
deadq_remove(pid_t pid)
{
	dq_t q;

	TAILQ_FOREACH(q, &deadq_head, dq_entries) {
		if (q->dq_pid == pid)
			break;
	}
	if (q != NULL) {
		TAILQ_REMOVE(&deadq_head, q, dq_entries);
		free(q);
		return (1);
	}

	return (0);
}

static void
log_deadchild(pid_t pid, int status, const char *name)
{
	int code;
	char buf[256];
	const char *reason;

	errno = 0; /* Keep strerror() stuff out of logerror messages. */
	if (WIFSIGNALED(status)) {
		reason = "due to signal";
		code = WTERMSIG(status);
	} else {
		reason = "with status";
		code = WEXITSTATUS(status);
		if (code == 0)
			return;
	}
	(void)snprintf(buf, sizeof buf,
		       "Logging subprocess %d (%s) exited %s %d.",
		       pid, name, reason, code);
	logerror(buf);
}

static int
socksetup(struct peer *pe)
{
	struct addrinfo hints, *res, *res0;
	struct socklist *sl;
	int error;
	char *cp;
	/*
	 * We have to handle this case for backwards compatibility:
	 * If there are two (or more) colons but no '[' and ']',
	 * assume this is an inet6 address without a service.
	 */
	if (pe->pe_name != NULL) {
#ifdef INET6
		if (pe->pe_name[0] == '[' &&
		    (cp = strchr(pe->pe_name + 1, ']')) != NULL) {
			pe->pe_name = &pe->pe_name[1];
			*cp = '\0';
			if (cp[1] == ':' && cp[2] != '\0')
				pe->pe_serv = cp + 2;
		} else {
#endif
			cp = strchr(pe->pe_name, ':');
			if (cp != NULL && strchr(cp + 1, ':') == NULL) {
				*cp = '\0';
				if (cp[1] != '\0')
					pe->pe_serv = cp + 1;
				if (cp == pe->pe_name)
					pe->pe_name = NULL;
			}
#ifdef INET6
		}
#endif
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	dprintf("Try %s\n", pe->pe_name);
	if (pe->pe_serv == NULL)
		pe->pe_serv = "syslog";
	error = getaddrinfo(pe->pe_name, pe->pe_serv, &hints, &res0);
	if (error) {
		logerror(gai_strerror(error));
		errno = 0;
		die(0);
	}
	for (res = res0; res != NULL; res = res->ai_next) {
		int s;
		int on = 1;

		if (res->ai_family == AF_LOCAL)
			unlink(pe->pe_name);
		else if (SecureMode > 1) {
			/* Only AF_LOCAL in secure mode. */
			continue;
		}
		if (family != AF_UNSPEC && res->ai_family != family)
			continue;

		s = socket(res->ai_family, res->ai_socktype,
		    res->ai_protocol);
		if (s < 0) {
			logerror("socket");
			error++;
			continue;
		}
#ifdef INET6
		if (res->ai_family == AF_INET6) {
			if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,
			       (char *)&on, sizeof(on)) < 0) {
				logerror("setsockopt(IPV6_V6ONLY)");
				close(s);
				error++;
				continue;
			}
		}
#endif
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&on, sizeof(on)) < 0) {
			logerror("setsockopt(SO_REUSEADDR)");
			close(s);
			error++;
			continue;
		}
		/*
		 * RFC 3164 recommends that client side message
		 * should come from the privileged syslogd port.
		 *
		 * If the system administrator choose not to obey
		 * this, we can skip the bind() step so that the
		 * system will choose a port for us.
		 */
		if (NoBind == 0) {
			if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {
				logerror("bind");
				close(s);
				error++;
				continue;
			}
			if (SecureMode == 0)
				increase_rcvbuf(s);
		}
		if (res->ai_family == AF_LOCAL &&
	    	    chmod(pe->pe_name, pe->pe_mode) < 0) {
			dprintf("chmod %s: %s\n", pe->pe_name,
			    strerror(errno));
			close(s);
			error++;
			continue;
		}
		dprintf("new socket fd is %d\n", s);
		listen(s, 5);
		dprintf("shutdown\n");
		if (SecureMode) {
			/* Forbid communication in secure mode. */
			if (shutdown(s, SHUT_RD) < 0 &&
			    errno != ENOTCONN) {
				logerror("shutdown");
				if (!Debug)
					die(0);
			}
			dprintf("listening on inet socket\n");
		} else
			dprintf("sending on inet socket\n");
		sl = calloc(1, sizeof(*sl));
		if (sl == NULL)
			err(1, "malloc failed");
		sl->sl_socket = s;
		memcpy(&sl->sl_ss, res->ai_addr, res->ai_addrlen);
		sl->sl_peer = pe;
		STAILQ_INSERT_TAIL(&shead, sl, next);
	}
	freeaddrinfo(res0);

	return(error);
}

static void
increase_rcvbuf(int fd)
{
	socklen_t len, slen;

	slen = sizeof(len);

	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, &slen) == 0) {
		if (len < RCVBUF_MINSIZE) {
			len = RCVBUF_MINSIZE;
			setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
		}
	}
}
