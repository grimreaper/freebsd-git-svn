/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Paul Vixie          <paul@vix.com>          uunet!decwrl!vixie!paul
 */

#if !defined(lint) && !defined(LINT)
static const char rcsid[] =
  "$FreeBSD$";
#endif

#define	MAIN_PROGRAM


#include "cron.h"
#include <sys/signal.h>
#if SYS_TIME_H
# include <sys/time.h>
#else
# include <time.h>
#endif


static	void	usage __P((void)),
		run_reboot_jobs __P((cron_db *)),
		cron_tick __P((cron_db *)),
		cron_sync __P((void)),
		cron_sleep __P((cron_db *)),
		cron_clean __P((cron_db *)),
#ifdef USE_SIGCHLD
		sigchld_handler __P((int)),
#endif
		sighup_handler __P((int)),
		parse_args __P((int c, char *v[]));

static time_t	last_time = 0;
static int	dst_enabled = 0;

static void
usage() {
    char **dflags;

	fprintf(stderr, "usage: cron [-s] [-x debugflag[,...]]\n");
	fprintf(stderr, "\ndebugflags: ");

        for(dflags = DebugFlagNames; *dflags; dflags++) {
		fprintf(stderr, "%s ", *dflags);
	}
        fprintf(stderr, "\n");

	exit(ERROR_EXIT);
}


int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	cron_db	database;
	ProgramName = argv[0];

#if defined(BSD)
	setlinebuf(stdout);
	setlinebuf(stderr);
#endif

	parse_args(argc, argv);

#ifdef USE_SIGCHLD
	(void) signal(SIGCHLD, sigchld_handler);
#else
	(void) signal(SIGCLD, SIG_IGN);
#endif
	(void) signal(SIGHUP, sighup_handler);

	acquire_daemonlock(0);
	set_cron_uid();
	set_cron_cwd();

#if defined(POSIX)
	setenv("PATH", _PATH_DEFPATH, 1);
#endif

	/* if there are no debug flags turned on, fork as a daemon should.
	 */
# if DEBUGGING
	if (DebugFlags) {
# else
	if (0) {
# endif
		(void) fprintf(stderr, "[%d] cron started\n", getpid());
	} else {
		switch (fork()) {
		case -1:
			log_it("CRON",getpid(),"DEATH","can't fork");
			exit(0);
			break;
		case 0:
			/* child process */
			log_it("CRON",getpid(),"STARTUP","fork ok");
			(void) setsid();
			break;
		default:
			/* parent process should just die */
			_exit(0);
		}
	}

	acquire_daemonlock(0);
	database.head = NULL;
	database.tail = NULL;
	database.mtime = (time_t) 0;
	load_database(&database);
	run_reboot_jobs(&database);
	cron_sync();
	while (TRUE) {
# if DEBUGGING
	    /* if (!(DebugFlags & DTEST)) */
# endif /*DEBUGGING*/
			cron_sleep(&database);

		load_database(&database);

		/* do this iteration
		 */
		cron_tick(&database);

		/* sleep 1 minute
		 */
		TargetTime += 60;
	}
}


static void
run_reboot_jobs(db)
	cron_db *db;
{
	register user		*u;
	register entry		*e;

	for (u = db->head;  u != NULL;  u = u->next) {
		for (e = u->crontab;  e != NULL;  e = e->next) {
			if (e->flags & WHEN_REBOOT) {
				job_add(e, u);
			}
		}
	}
	(void) job_runqueue();
}


static void
cron_tick(db)
	cron_db	*db;
{
	static struct tm lasttm;
 	register struct tm	*tm = localtime(&TargetTime);
	register int		minute, hour, dom, month, dow;
	register user		*u;
	register entry		*e;

	/* make 0-based values out of these so we can use them as indicies
	 */
	minute = tm->tm_min -FIRST_MINUTE;
	hour = tm->tm_hour -FIRST_HOUR;
	dom = tm->tm_mday -FIRST_DOM;
	month = tm->tm_mon +1 /* 0..11 -> 1..12 */ -FIRST_MONTH;
	dow = tm->tm_wday -FIRST_DOW;

	Debug(DSCH, ("[%d] tick(%d,%d,%d,%d,%d)\n",
		getpid(), minute, hour, dom, month, dow))

	/* check for the daylight saving time change 
	 * we support only change by +-1 hour happening at :00 minutes,
	 * those living in more strange timezones are out of luck
	 */
	if (dst_enabled && last_time != 0 
	&& TargetTime > last_time /* exclude stepping back */
	&& tm->tm_isdst != lasttm.tm_isdst ) {
		int	prevhr, nexthr, runtime;
		int	lastmin, lasthour; 
		int	trandom, tranmonth, trandow;
		time_t	diff; /* time difference in seconds */

		lastmin = lasttm.tm_min -FIRST_MINUTE;
		lasthour = lasttm.tm_hour -FIRST_HOUR;

		prevhr = (hour + (HOUR_COUNT-1)) % HOUR_COUNT;
		nexthr = (lasthour + 1) % HOUR_COUNT;

		if ( lasttm.tm_isdst != 1 && tm->tm_isdst == 1 /* ST->DST */
		&& prevhr == nexthr ) { 
			diff = ( (hour*MINUTE_COUNT + minute)
				- (lasthour*MINUTE_COUNT + lastmin) 
				+ HOUR_COUNT*MINUTE_COUNT
				) % (HOUR_COUNT*MINUTE_COUNT);
			diff -=  (TargetTime - last_time) / 60/*seconds*/;
			if (diff != MINUTE_COUNT) 
				goto dstdone;

			if (hour == 0) {
				trandom = lasttm.tm_mday -FIRST_DOM;
				tranmonth = lasttm.tm_mon +1 /* 0..11 -> 1..12 */ -FIRST_MONTH;
				trandow = lasttm.tm_wday -FIRST_DOW;
			} else {
				trandom = dom;
				tranmonth = month;
				trandow = dow;
			}

			for (u = db->head;  u != NULL;  u = u->next) {
				for (e = u->crontab;  e != NULL;  e = e->next) {
					/* adjust only jobs less frequent than 1 hr */
					if ( !bit_test(e->hour, prevhr)
					|| bit_test(e->hour, lasthour)
					|| bit_test(e->hour, hour) )
						continue;

					if ( bit_test(e->month, tranmonth)
					&& ( ((e->flags & DOM_STAR) || (e->flags & DOW_STAR))
					 ? (bit_test(e->dow,trandow) && bit_test(e->dom,trandom))
					 : (bit_test(e->dow,trandow) || bit_test(e->dom,trandom)) )
					) {
						bit_ffs(e->minute, MINUTE_COUNT, &runtime);
						if(runtime >= 0) {
							e->tmval = TargetTime + (runtime-minute)*60;
							e->flags |= RUN_AT;
							e->flags &= ~NOT_UNTIL;
						}
					}
				}
			}
		} else if ( lasttm.tm_isdst == 1 && tm->tm_isdst != 1 /* DST->ST */
		&& lasthour == hour ) { 
			diff = ( (lasthour*MINUTE_COUNT + lastmin)
				- (hour*MINUTE_COUNT + minute) 
				+ HOUR_COUNT*MINUTE_COUNT
				) % (HOUR_COUNT*MINUTE_COUNT);
			diff +=  (TargetTime - last_time) / 60/*seconds*/;
			if (diff != MINUTE_COUNT) 
				goto dstdone;

			runtime = TargetTime + (MINUTE_COUNT  - minute)*60;
			for (u = db->head;  u != NULL;  u = u->next) {
				for (e = u->crontab;  e != NULL;  e = e->next) {
					/* adjust only jobs less frequent than 1 hr */
					if ( !bit_test(e->hour, hour)
					|| bit_test(e->hour, prevhr)
					|| bit_test(e->hour, nexthr) )
						continue;

					e->tmval = runtime;
					e->flags |= NOT_UNTIL;
					e->flags &= ~RUN_AT;
				}
			}
		}
	}
dstdone:

	/* the dom/dow situation is odd.  '* * 1,15 * Sun' will run on the
	 * first and fifteenth AND every Sunday;  '* * * * Sun' will run *only*
	 * on Sundays;  '* * 1,15 * *' will run *only* the 1st and 15th.  this
	 * is why we keep 'e->dow_star' and 'e->dom_star'.  yes, it's bizarre.
	 * like many bizarre things, it's the standard.
	 */
	for (u = db->head;  u != NULL;  u = u->next) {
		for (e = u->crontab;  e != NULL;  e = e->next) {
			Debug(DSCH|DEXT, ("user [%s:%d:%d:...] cmd=\"%s\"\n",
					  env_get("LOGNAME", e->envp),
					  e->uid, e->gid, e->cmd))
			if (e->flags & NOT_UNTIL) {
				if (TargetTime >= e->tmval)
					e->flags &= ~NOT_UNTIL;
				else
					continue;
			}
			if ( (e->flags & RUN_AT) && TargetTime == e->tmval
			|| bit_test(e->minute, minute)
			 && bit_test(e->hour, hour)
			 && bit_test(e->month, month)
			 && ( ((e->flags & DOM_STAR) || (e->flags & DOW_STAR))
			      ? (bit_test(e->dow,dow) && bit_test(e->dom,dom))
			      : (bit_test(e->dow,dow) || bit_test(e->dom,dom))
			    )
			   ) {
				e->flags &= ~RUN_AT;
				job_add(e, u);
			}
		}
	}

	last_time = TargetTime;
	lasttm = *tm;
}


/* the task here is to figure out how long it's going to be until :00 of the
 * following minute and initialize TargetTime to this value.  TargetTime
 * will subsequently slide 60 seconds at a time, with correction applied
 * implicitly in cron_sleep().  it would be nice to let cron execute in
 * the "current minute" before going to sleep, but by restarting cron you
 * could then get it to execute a given minute's jobs more than once.
 * instead we have the chance of missing a minute's jobs completely, but
 * that's something sysadmin's know to expect what with crashing computers..
 */
static void
cron_sync() {
 	register struct tm	*tm;

	TargetTime = time((time_t*)0);
	tm = localtime(&TargetTime);
	TargetTime += (60 - tm->tm_sec);
}


static void
cron_sleep(db)
	cron_db	*db;
{
	int	seconds_to_wait = 0;

	/*
	 * Loop until we reach the top of the next minute, sleep when possible.
	 */

	for (;;) {
		seconds_to_wait = (int) (TargetTime - time((time_t*)0));

		/*
		 * If the seconds_to_wait value is insane, jump the cron
		 */

		if (seconds_to_wait < -600 || seconds_to_wait > 600) {
			cron_clean(db);
			cron_sync();
			continue;
		}

		Debug(DSCH, ("[%d] TargetTime=%ld, sec-to-wait=%d\n",
			getpid(), (long)TargetTime, seconds_to_wait))

		/*
		 * If we've run out of wait time or there are no jobs left
		 * to run, break
		 */

		if (seconds_to_wait <= 0)
			break;
		if (job_runqueue() == 0) {
			Debug(DSCH, ("[%d] sleeping for %d seconds\n",
				getpid(), seconds_to_wait))

			sleep(seconds_to_wait);
		}
	}
}


/* if the time was changed abruptly, clear the flags related
 * to the daylight time switch handling to avoid strange effects
 */

static void
cron_clean(db)
	cron_db	*db;
{
	user		*u;
	entry		*e;

	last_time = 0;

	for (u = db->head;  u != NULL;  u = u->next) {
		for (e = u->crontab;  e != NULL;  e = e->next) {
			e->flags &= ~(RUN_AT|NOT_UNTIL);
		}
	}
}

#ifdef USE_SIGCHLD
static void
sigchld_handler(x) {
	WAIT_T		waiter;
	PID_T		pid;

	for (;;) {
#ifdef POSIX
		pid = waitpid(-1, &waiter, WNOHANG);
#else
		pid = wait3(&waiter, WNOHANG, (struct rusage *)0);
#endif
		switch (pid) {
		case -1:
			Debug(DPROC,
				("[%d] sigchld...no children\n", getpid()))
			return;
		case 0:
			Debug(DPROC,
				("[%d] sigchld...no dead kids\n", getpid()))
			return;
		default:
			Debug(DPROC,
				("[%d] sigchld...pid #%d died, stat=%d\n",
				getpid(), pid, WEXITSTATUS(waiter)))
		}
	}
}
#endif /*USE_SIGCHLD*/


static void
sighup_handler(x) {
	log_close();
}


static void
parse_args(argc, argv)
	int	argc;
	char	*argv[];
{
	int	argch;

	while ((argch = getopt(argc, argv, "sx:")) != -1) {
		switch (argch) {
		case 's':
			dst_enabled = 1;
			break;
		case 'x':
			if (!set_debug_flags(optarg))
				usage();
			break;
		default:
			usage();
		}
	}
}
