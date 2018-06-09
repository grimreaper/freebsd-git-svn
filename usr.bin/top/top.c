/*-
 *  Top users/processes display for Unix
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989 - 1994, William LeFebvre, Northwestern University
 *  Copyright (c) 1994, 1995, William LeFebvre, Argonne National Laboratory
 *  Copyright (c) 1996, William LeFebvre, Group sys Consulting
 *
 * $FreeBSD$
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/cdefs.h>
#include <sys/limits.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <time.h>

#include <errno.h>
#include <jail.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"
#include "display.h"		/* interface to display package */
#include "screen.h"		/* interface to screen package */
#include "top.h"
#include "machine.h"
#include "utils.h"
#include "username.h"

/* Size of the stdio buffer given to stdout */
#define Buffersize	2048

char copyright[] =
    "Copyright (c) 1984 through 1996, William LeFebvre";

typedef void sigret_t;

/* The buffer that stdio will use */
static char stdoutbuf[Buffersize];

/* build Signal masks */
#define Smask(s)	(1 << ((s) - 1))


static int fmt_flags = 0;
int pcpu_stats = false;

/* signal handling routines */
static sigret_t leave(int);
static sigret_t tstop(int);
static sigret_t top_winch(int);

static volatile sig_atomic_t leaveflag;
static volatile sig_atomic_t tstopflag;
static volatile sig_atomic_t winchflag;

/* values which need to be accessed by signal handlers */
static int max_topn;		/* maximum displayable processes */

/* miscellaneous things */
struct process_select ps;
const char * myname = "top";
pid_t mypid;

/* pointers to display routines */
static void (*d_loadave)(int mpid, double *avenrun) = i_loadave;
static void (*d_procstates)(int total, int *brkdn) = i_procstates;
static void (*d_cpustates)(int *states) = i_cpustates;
static void (*d_memory)(int *stats) = i_memory;
static void (*d_arc)(int *stats) = i_arc;
static void (*d_carc)(int *stats) = i_carc;
static void (*d_swap)(int *stats) = i_swap;
static void (*d_message)(void) = i_message;
static void (*d_header)(const char *text) = i_header;
static void (*d_process)(int line, char *thisline) = i_process;

static void reset_display(void);

static void
reset_uids(void)
{
    for (size_t i = 0; i < TOP_MAX_UIDS; ++i)
	ps.uid[i] = -1;
}

static int
add_uid(int uid)
{
    size_t i = 0;

    /* Add the uid if there's room */
    for (; i < TOP_MAX_UIDS; ++i)
    {
	if (ps.uid[i] == -1 || ps.uid[i] == uid)
	{
	    ps.uid[i] = uid;
	    break;
	}
    }

    return (i == TOP_MAX_UIDS);
}

static void
rem_uid(int uid)
{
    size_t i = 0;
    size_t where = TOP_MAX_UIDS;

    /* Look for the user to remove - no problem if it's not there */
    for (; i < TOP_MAX_UIDS; ++i)
    {
	if (ps.uid[i] == -1)
	    break;
	if (ps.uid[i] == uid)
	    where = i;
    }

    /* Make sure we don't leave a hole in the middle */
    if (where != TOP_MAX_UIDS)
    {
	ps.uid[where] = ps.uid[i-1];
	ps.uid[i-1] = -1;
    }
}

static int
handle_user(char *buf, size_t buflen)
{
    int rc = 0;
    int uid = -1;
    char *buf2 = buf;

    new_message(MT_standout, "Username to show (+ for all): ");
    if (readline(buf, buflen, false) <= 0)
    {
	clear_message();
	return (rc);
    }

    if (buf[0] == '+' || buf[0] == '-')
    {
	if (buf[1] == '\0')
	{
	    reset_uids();
	    goto end;
	}
	else
	    ++buf2;
    }

    if ((uid = userid(buf2)) == -1)
    {
	new_message(MT_standout, " %s: unknown user", buf2);
	rc = 1;
	goto end;
    }

    if (buf2 == buf)
    {
	reset_uids();
	ps.uid[0] = uid;
	goto end;
    }

    if (buf[0] == '+')
    {
	if (add_uid(uid))
	{
	    new_message(MT_standout, " too many users, reset with '+'");
	    rc = 1;
	    goto end;
	}
    }
    else
	rem_uid(uid);

end:
    putchar('\r');
    return (rc);
}

int
main(int argc, char *argv[])
{
    int i;
    int active_procs;
    int change;

    struct system_info system_info;
    struct statics statics;
    void * processes;

    static char tempbuf1[50];
    static char tempbuf2[50];
    int old_sigmask;		/* only used for BSD-style signals */
    int topn = Infinity;
    int delay = Default_DELAY;
    int displays = 0;		/* indicates unspecified */
    int sel_ret = 0;
    time_t curr_time;
    char *(*get_userid)(int) = username;
    const char *uname_field = "USERNAME";
    const char *header_text;
    char *env_top;
    const char **preset_argv;
    int  preset_argc = 0;
    const char **av;
    int  ac;
    bool dostates = false;
    bool do_unames = true;
    char interactive = 2;
    char warnings = 0;
    char topn_specified = false;
    char ch;
    char *iptr;
    char no_command = 1;
    struct timeval timeout;
    char *order_name = NULL;
    int order_index = 0;
    fd_set readfds;
    char old_system = false;

    static const char command_chars[] = "\f qh?en#sdkriIutHmSCajzPJwopT";
/* these defines enumerate the "strchr"s of the commands in command_chars */
#define CMD_redraw	0
#define CMD_update	1
#define CMD_quit	2
#define CMD_help1	3
#define CMD_help2	4
#define CMD_OSLIMIT	4    /* terminals with OS can only handle commands */
#define CMD_errors	5    /* less than or equal to CMD_OSLIMIT	   */
#define CMD_number1	6
#define CMD_number2	7
#define CMD_delay	8
#define CMD_displays	9
#define CMD_kill	10
#define CMD_renice	11
#define CMD_idletog     12
#define CMD_idletog2    13
#define CMD_user	14
#define CMD_selftog	15
#define CMD_thrtog	16
#define CMD_viewtog	17
#define CMD_viewsys	18
#define	CMD_wcputog	19
#define	CMD_showargs	20
#define	CMD_jidtog	21
#define CMD_kidletog	22
#define CMD_pcputog	23
#define CMD_jail	24
#define CMD_swaptog	25
#define CMD_order	26
#define CMD_pid		27
#define CMD_toggletid	28

_Static_assert(sizeof(command_chars) == CMD_toggletid + 2, "command chars size");

    /* set the buffer for stdout */
#ifdef DEBUG
    extern FILE *debug;
    debug = fopen("debug.run", "w");
    setbuffer(stdout, NULL, 0);
#else
    setbuffer(stdout, stdoutbuf, Buffersize);
#endif

    if (argc > 0)
    {
	if ((myname = strrchr(argv[0], '/')) == 0)
	{
	    myname = argv[0];
	}
	else
	{
	    myname++;
	}
    }

    mypid = getpid();

    /* get our name */
    /* initialize some selection options */
    ps.idle    = true;
    ps.self    = false;
    ps.system  = false;
    reset_uids();
    ps.thread  = false;
    ps.wcpu    = 1;
    ps.jid     = -1;
    ps.jail    = false;
    ps.swap    = false;
    ps.kidle   = true;
    ps.pid     = -1; 
    ps.command = NULL;
    ps.thread_id = false;

    /* get preset options from the environment */
    if ((env_top = getenv("TOP")) != NULL)
    {
	av = preset_argv = argparse(env_top, &preset_argc);
	ac = preset_argc;

	/* set the dummy argument to an explanatory message, in case
	   getopt encounters a bad argument */
	preset_argv[0] = "while processing environment";
    }

    /* process options */
    do {
	/* if we're done doing the presets, then process the real arguments */
	if (preset_argc == 0)
	{
	    ac = argc;
	    av = argv;

	    /* this should keep getopt happy... */
	    optind = 1;
	}

	while ((i = getopt(ac, av, "CSIHPabijJ:nquvzs:d:U:m:o:p:Ttw")) != EOF)
	{
	    switch(i)
	    {
	      case 'v':			/* show version number */
		fprintf(stderr, "%s: version FreeBSD\n", myname);
		exit(1);
		break;

	      case 'u':			/* toggle uid/username display */
		do_unames = !do_unames;
		break;

	      case 'U':			/* display only username's processes */
		if ((ps.uid[0] = userid(optarg)) == -1)
		{
		    fprintf(stderr, "%s: unknown user\n", optarg);
		    exit(1);
		}
		break;

	      case 'S':			/* show system processes */
		ps.system = true;
		old_system = true;
		break;

	      case 'I':                   /* show idle processes */
		ps.idle = !ps.idle;
		break;

	      case 'i':			/* go interactive regardless */
		interactive = 1;
		break;

	      case 'n':			/* batch, or non-interactive */
	      case 'b':
		interactive = 0;
		break;

	      case 'a':
		fmt_flags ^= FMT_SHOWARGS;
		break;

	      case 'd':			/* number of displays to show */
		if ((i = atoiwi(optarg)) == Invalid || i == 0)
		{
		    fprintf(stderr,
			"%s: warning: display count should be positive -- option ignored\n",
			myname);
		    warnings++;
		}
		else
		{
		    displays = i;
		}
		break;
	      case 'p': {
		unsigned long long num;
		const char *errstr;

		num = strtonum(optarg, 0, INT_MAX, &errstr);
		if (errstr != NULL || !find_pid(num)) {
			fprintf(stderr, "%s: unknown pid\n", optarg);
			exit(1);
		}
		ps.pid = (pid_t)num;
		ps.system = true;
		break;
	      }

	      case 's':
		if ((delay = atoi(optarg)) < 0 || (delay == 0 && getuid() != 0))
		{
		    fprintf(stderr,
			"%s: warning: seconds delay should be positive -- using default\n",
			myname);
		    delay = Default_DELAY;
		    warnings++;
		}
		break;

	      case 'q':		/* be quick about it */
		/* only allow this if user is really root */
		if (getuid() == 0)
		{
		    /* be very un-nice! */
		    nice(-20);
		}
		else
		{
		    fprintf(stderr,
			"%s: warning: `-q' option can only be used by root\n",
			myname);
		    warnings++;
		}
		break;

	      case 'm':		/* select display mode */
		if (strcmp(optarg, "io") == 0) {
			displaymode = DISP_IO;
		} else if (strcmp(optarg, "cpu") == 0) {
			displaymode = DISP_CPU;
		} else {
			fprintf(stderr,
			"%s: warning: `-m' option can only take args "
			"'io' or 'cpu'\n",
			myname);
			exit(1);
		}
		break;

	      case 'o':		/* select sort order */
		order_name = optarg;
		break;

	      case 't':
		ps.self = !ps.self;
		break;

	      case 'C':
		ps.wcpu = !ps.wcpu;
		break;

	      case 'H':
		ps.thread = !ps.thread;
		break;

	      case 'T':
		ps.thread_id = !ps.thread_id;
		break;

	      case 'j':
		ps.jail = !ps.jail;
		break;

	      case 'J':			/* display only jail's processes */
		if ((ps.jid = jail_getid(optarg)) == -1)
		{
		    fprintf(stderr, "%s: unknown jail\n", optarg);
		    exit(1);
		}
		ps.jail = 1;
		break;

	      case 'P':
		pcpu_stats = !pcpu_stats;
		break;

	      case 'w':
		ps.swap = 1;
		break;

	      case 'z':
		ps.kidle = !ps.kidle;
		break;

	      default:
		fprintf(stderr,
"Usage: %s [-abCHIijnPqStuvwz] [-d count] [-m io | cpu] [-o field] [-p pid]\n"
"       [-s time] [-J jail] [-U username] [number]\n",
			myname);
		exit(1);
	    }
	}

	/* get count of top processes to display (if any) */
	if (optind < ac)
	{
	    if ((topn = atoiwi(av[optind])) == Invalid)
	    {
		fprintf(stderr,
			"%s: warning: process display count should be non-negative -- using default\n",
			myname);
		warnings++;
	    }
            else
	    {
		topn_specified = true;
	    }
	}

	/* tricky:  remember old value of preset_argc & set preset_argc = 0 */
	i = preset_argc;
	preset_argc = 0;

    /* repeat only if we really did the preset arguments */
    } while (i != 0);

    /* set constants for username/uid display correctly */
    if (!do_unames)
    {
	uname_field = "   UID  ";
	get_userid = itoa7;
    }

    /* initialize the kernel memory interface */
    if (machine_init(&statics) == -1)
    {
	exit(1);
    }

    /* determine sorting order index, if necessary */
    if (order_name != NULL)
    {
	if ((order_index = string_index(order_name, statics.order_names)) == -1)
	{
	    const char * const *pp;

	    fprintf(stderr, "%s: '%s' is not a recognized sorting order.\n",
		    myname, order_name);
	    fprintf(stderr, "\tTry one of these:");
	    pp = statics.order_names;
	    while (*pp != NULL)
	    {
		fprintf(stderr, " %s", *pp++);
	    }
	    fputc('\n', stderr);
	    exit(1);
	}
    }

    /* initialize termcap */
    init_termcap(interactive);

    /* get the string to use for the process area header */
    header_text = format_header(uname_field);

    /* initialize display interface */
    if ((max_topn = display_init(&statics)) == -1)
    {
	fprintf(stderr, "%s: can't allocate sufficient memory\n", myname);
	exit(4);
    }
    
    /* print warning if user requested more processes than we can display */
    if (topn > max_topn)
    {
	fprintf(stderr,
		"%s: warning: this terminal can only display %d processes.\n",
		myname, max_topn);
	warnings++;
    }

    /* adjust for topn == Infinity */
    if (topn == Infinity)
    {
	/*
	 *  For smart terminals, infinity really means everything that can
	 *  be displayed, or Largest.
	 *  On dumb terminals, infinity means every process in the system!
	 *  We only really want to do that if it was explicitly specified.
	 *  This is always the case when "Default_TOPN != Infinity".  But if
	 *  topn wasn't explicitly specified and we are on a dumb terminal
	 *  and the default is Infinity, then (and only then) we use
	 *  "Nominal_TOPN" instead.
	 */
	topn = smart_terminal ? Largest :
		    (topn_specified ? Largest : Nominal_TOPN);
    }

    /* set header display accordingly */
    display_header(topn > 0);

    /* determine interactive state */
    if (interactive == 2)
    {
	interactive = smart_terminal;
    }

    /* if # of displays not specified, fill it in */
    if (displays == 0)
    {
	displays = smart_terminal ? Infinity : 1;
    }

    /* hold interrupt signals while setting up the screen and the handlers */
    old_sigmask = sigblock(Smask(SIGINT) | Smask(SIGQUIT) | Smask(SIGTSTP));
    init_screen();
    signal(SIGINT, leave);
    signal(SIGQUIT, leave);
    signal(SIGTSTP, tstop);
    signal(SIGWINCH, top_winch);
    sigsetmask(old_sigmask);
    if (warnings)
    {
	fputs("....", stderr);
	fflush(stderr);			/* why must I do this? */
	sleep((unsigned)(3 * warnings));
	fputc('\n', stderr);
    }

restart:

    /*
     *  main loop -- repeat while display count is positive or while it
     *		indicates infinity (by being -1)
     */

    while ((displays == -1) || (displays-- > 0))
    {
	int (*compare)(const void * const, const void * const);

	    
	/* get the current stats */
	get_system_info(&system_info);

	compare = compares[order_index];

	/* get the current set of processes */
	processes =
		get_process_info(&system_info, &ps, compare);

	/* display the load averages */
	(*d_loadave)(system_info.last_pid,
		     system_info.load_avg);

	/* display the current time */
	/* this method of getting the time SHOULD be fairly portable */
	time(&curr_time);
	i_uptime(&system_info.boottime, &curr_time);
	i_timeofday(&curr_time);

	/* display process state breakdown */
	(*d_procstates)(system_info.p_total,
			system_info.procstates);

	/* display the cpu state percentage breakdown */
	if (dostates)	/* but not the first time */
	{
	    (*d_cpustates)(system_info.cpustates);
	}
	else
	{
	    /* we'll do it next time */
	    if (smart_terminal)
	    {
		z_cpustates();
	    }
	    else
	    {
		putchar('\n');
	    }
	    dostates = true;
	}

	/* display memory stats */
	(*d_memory)(system_info.memory);
	(*d_arc)(system_info.arc);
	(*d_carc)(system_info.carc);

	/* display swap stats */
	(*d_swap)(system_info.swap);

	/* handle message area */
	(*d_message)();

	/* update the header area */
	(*d_header)(header_text);
    
	if (topn > 0)
	{
	    /* determine number of processes to actually display */
	    /* this number will be the smallest of:  active processes,
	       number user requested, number current screen accomodates */
	    active_procs = system_info.p_pactive;
	    if (active_procs > topn)
	    {
		active_procs = topn;
	    }
	    if (active_procs > max_topn)
	    {
		active_procs = max_topn;
	    }

	    /* now show the top "n" processes. */
	    for (i = 0; i < active_procs; i++)
	    {
		(*d_process)(i, format_next_process(processes, get_userid,
			     fmt_flags));
	    }
	}
	else
	{
	    i = 0;
	}

	/* do end-screen processing */
	u_endscreen(i);

	/* now, flush the output buffer */
	if (fflush(stdout) != 0)
	{
	    new_message(MT_standout, " Write error on stdout");
	    putchar('\r');
	    quit(1);
	}

	/* only do the rest if we have more displays to show */
	if (displays)
	{
	    /* switch out for new display on smart terminals */
	    if (smart_terminal)
	    {
		if (overstrike)
		{
		    reset_display();
		}
		else
		{
		    d_loadave = u_loadave;
		    d_procstates = u_procstates;
		    d_cpustates = u_cpustates;
		    d_memory = u_memory;
		    d_arc = u_arc;
		    d_carc = u_carc;
		    d_swap = u_swap;
		    d_message = u_message;
		    d_header = u_header;
		    d_process = u_process;
		}
	    }
    
	    no_command = true;
	    if (!interactive)
	    {
		sleep(delay);
		if (leaveflag) {
		    end_screen();
		    exit(0);
		}
	    }
	    else while (no_command)
	    {
		/* assume valid command unless told otherwise */
		no_command = false;

		/* set up arguments for select with timeout */
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);		/* for standard input */
		timeout.tv_sec  = delay;
		timeout.tv_usec = 0;

		if (leaveflag) {
		    end_screen();
		    exit(0);
		}

		if (tstopflag) {
		    /* move to the lower left */
		    end_screen();
		    fflush(stdout);

		    /* default the signal handler action */
		    signal(SIGTSTP, SIG_DFL);

		    /* unblock the signal and send ourselves one */
		    sigsetmask(sigblock(0) & ~(1 << (SIGTSTP - 1)));
		    kill(0, SIGTSTP);

		    /* reset the signal handler */
		    signal(SIGTSTP, tstop);

		    /* reinit screen */
		    reinit_screen();
		    reset_display();
		    tstopflag = 0;
		    goto restart;
		}

		if (winchflag) {
		    /* reascertain the screen dimensions */
		    get_screensize();

		    /* tell display to resize */
		    max_topn = display_resize();

		    /* reset the signal handler */
		    signal(SIGWINCH, top_winch);

		    reset_display();
		    winchflag = 0;
		    goto restart;
		}

		/* wait for either input or the end of the delay period */
		sel_ret = select(2, &readfds, NULL, NULL, &timeout);
		if (sel_ret < 0 && errno != EINTR)
		    quit(0);
		if (sel_ret > 0)
		{
		    int newval;
		    const char *errmsg;
    
		    /* something to read -- clear the message area first */
		    clear_message();

		    /* now read it and convert to command strchr */
		    /* (use "change" as a temporary to hold strchr) */
		    if (read(0, &ch, 1) != 1)
		    {
			/* read error: either 0 or -1 */
			new_message(MT_standout, " Read error on stdin");
			putchar('\r');
			quit(1);
		    }
		    if ((iptr = strchr(command_chars, ch)) == NULL)
		    {
			if (ch != '\r' && ch != '\n')
			{
			    /* illegal command */
			    new_message(MT_standout, " Command not understood");
			}
			putchar('\r');
			no_command = true;
		    }
		    else
		    {
			change = iptr - command_chars;
			if (overstrike && change > CMD_OSLIMIT)
			{
			    /* error */
			    new_message(MT_standout,
			    " Command cannot be handled by this terminal");
			    putchar('\r');
			    no_command = true;
			}
			else switch(change)
			{
			    case CMD_redraw:	/* redraw screen */
				reset_display();
				break;
    
			    case CMD_update:	/* merely update display */
				/* is the load average high? */
				if (system_info.load_avg[0] > LoadMax)
				{
				    /* yes, go home for visual feedback */
				    go_home();
				    fflush(stdout);
				}
				break;
	    
			    case CMD_quit:	/* quit */
				quit(0);
				break;
	    
			    case CMD_help1:	/* help */
			    case CMD_help2:
				reset_display();
				top_clear();
				show_help();
				top_standout("Hit any key to continue: ");
				fflush(stdout);
				read(0, &ch, 1);
				break;
	
			    case CMD_errors:	/* show errors */
				if (error_count() == 0)
				{
				    new_message(MT_standout,
					" Currently no errors to report.");
				    putchar('\r');
				    no_command = true;
				}
				else
				{
				    reset_display();
				    top_clear();
				    show_errors();
				    top_standout("Hit any key to continue: ");
				    fflush(stdout);
				    read(0, &ch, 1);
				}
				break;
	
			    case CMD_number1:	/* new number */
			    case CMD_number2:
				new_message(MT_standout,
				    "Number of processes to show: ");
				newval = readline(tempbuf1, 8, true);
				if (newval > -1)
				{
				    if (newval > max_topn)
				    {
					new_message(MT_standout | MT_delayed,
					  " This terminal can only display %d processes.",
					  max_topn);
					putchar('\r');
				    }

				    if (newval == 0)
				    {
					/* inhibit the header */
					display_header(false);
				    }
				    else if (newval > topn && topn == 0)
				    {
					/* redraw the header */
					display_header(true);
					d_header = i_header;
				    }
				    topn = newval;
				}
				break;
	    
			    case CMD_delay:	/* new seconds delay */
				new_message(MT_standout, "Seconds to delay: ");
				if ((i = readline(tempbuf1, 8, true)) > -1)
				{
				    if ((delay = i) == 0 && getuid() != 0)
				    {
					delay = 1;
				    }
				}
				clear_message();
				break;
	
			    case CMD_displays:	/* change display count */
				new_message(MT_standout,
					"Displays to show (currently %s): ",
					displays == -1 ? "infinite" :
							 itoa(displays));
				if ((i = readline(tempbuf1, 10, true)) > 0)
				{
				    displays = i;
				}
				else if (i == 0)
				{
				    quit(0);
				}
				clear_message();
				break;
    
			    case CMD_kill:	/* kill program */
				new_message(0, "kill ");
				if (readline(tempbuf2, sizeof(tempbuf2), false) > 0)
				{
				    if ((errmsg = kill_procs(tempbuf2)) != NULL)
				    {
					new_message(MT_standout, "%s", errmsg);
					putchar('\r');
					no_command = true;
				    }
				}
				else
				{
				    clear_message();
				}
				break;
	    
			    case CMD_renice:	/* renice program */
				new_message(0, "renice ");
				if (readline(tempbuf2, sizeof(tempbuf2), false) > 0)
				{
				    if ((errmsg = renice_procs(tempbuf2)) != NULL)
				    {
					new_message(MT_standout, "%s", errmsg);
					putchar('\r');
					no_command = true;
				    }
				}
				else
				{
				    clear_message();
				}
				break;

			    case CMD_idletog:
			    case CMD_idletog2:
				ps.idle = !ps.idle;
				new_message(MT_standout | MT_delayed,
				    " %sisplaying idle processes.",
				    ps.idle ? "D" : "Not d");
				putchar('\r');
				break;

			    case CMD_selftog:
				ps.self = !ps.self;
				new_message(MT_standout | MT_delayed,
				    " %sisplaying self.",
				    (ps.self) ? "D" : "Not d");
				putchar('\r');
				break;

			    case CMD_user:
				if (handle_user(tempbuf2, sizeof(tempbuf2)))
				    no_command = true;
				break;
	    
			    case CMD_thrtog:
				ps.thread = !ps.thread;
				new_message(MT_standout | MT_delayed,
				    " Displaying threads %s",
				    ps.thread ? "separately" : "as a count");
				header_text = format_header(uname_field);
				reset_display();
				putchar('\r');
				break;

			    case CMD_toggletid:
				ps.thread_id = !ps.thread_id;
				new_message(MT_standout | MT_delayed,
				    " Displaying %s",
				    ps.thread_id ? "tid" : "pid");
				header_text = format_header(uname_field);
				reset_display();
				putchar('\r');
				break;

			    case CMD_wcputog:
				ps.wcpu = !ps.wcpu;
				new_message(MT_standout | MT_delayed,
				    " Displaying %s CPU",
				    ps.wcpu ? "weighted" : "raw");
				header_text = format_header(uname_field);
				reset_display();
				putchar('\r');
				break;
			    case CMD_viewtog:
				if (++displaymode == DISP_MAX)
					displaymode = 0;
				header_text = format_header(uname_field);
				display_header(true);
				d_header = i_header;
				reset_display();
				break;
			    case CMD_viewsys:
				ps.system = !ps.system;
				old_system = ps.system;
				break;
			    case CMD_showargs:
				fmt_flags ^= FMT_SHOWARGS;
				break;
			    case CMD_order:
				new_message(MT_standout,
				    "Order to sort: ");
				if (readline(tempbuf2, sizeof(tempbuf2), false) > 0)
				{
				  if ((i = string_index(tempbuf2, statics.order_names)) == -1)
					{
					  new_message(MT_standout,
					      " %s: unrecognized sorting order", tempbuf2);
					  no_command = true;
				    }
				    else
				    {
					order_index = i;
				    }
				    putchar('\r');
				}
				else
				{
				    clear_message();
				}
				break;
			    case CMD_jidtog:
				ps.jail = !ps.jail;
				new_message(MT_standout | MT_delayed,
				    " %sisplaying jail ID.",
				    ps.jail ? "D" : "Not d");
				header_text = format_header(uname_field);
				reset_display();
				putchar('\r');
				break;

			    case CMD_jail:
				new_message(MT_standout,
				    "Jail to show (+ for all): ");
				if (readline(tempbuf2, sizeof(tempbuf2), false) > 0)
				{
				    if (tempbuf2[0] == '+' &&
					tempbuf2[1] == '\0')
				    {
					ps.jid = -1;
				    }
				    else if ((i = jail_getid(tempbuf2)) == -1)
				    {
					new_message(MT_standout,
					    " %s: unknown jail", tempbuf2);
					no_command = true;
				    }
				    else
				    {
					ps.jid = i;
				    }
				    if (ps.jail == 0) {
					    ps.jail = 1;
					    new_message(MT_standout |
						MT_delayed, " Displaying jail "
						"ID.");
					    header_text =
						format_header(uname_field);
					    reset_display();
				    }
				    putchar('\r');
				}
				else
				{
				    clear_message();
				}
				break;
	    
			    case CMD_kidletog:
				ps.kidle = !ps.kidle;
				new_message(MT_standout | MT_delayed,
				    " %sisplaying system idle process.",
				    ps.kidle ? "D" : "Not d");
				putchar('\r');
				break;
			    case CMD_pcputog:
				pcpu_stats = !pcpu_stats;
				new_message(MT_standout | MT_delayed,
				    " Displaying %sCPU statistics.",
				    pcpu_stats ? "per-" : "global ");
				toggle_pcpustats();
				max_topn = display_updatecpus(&statics);
				reset_display();
				putchar('\r');
				break;
			    case CMD_swaptog:
				ps.swap = !ps.swap;
				new_message(MT_standout | MT_delayed,
				    " %sisplaying per-process swap usage.",
				    ps.swap ? "D" : "Not d");
				header_text = format_header(uname_field);
				reset_display();
				putchar('\r');
				break;
			    case CMD_pid:
				new_message(MT_standout,
					"Process id to show (+ for all): ");
				if (readline(tempbuf2, sizeof(tempbuf2), false) > 0) {
					if (tempbuf2[0] == '+' &&
                   			    tempbuf2[1] == '\0') {
						ps.pid = (pid_t)-1;
						ps.system = old_system;
					} else {
						unsigned long long num;
						const char *errstr;

						num = strtonum(tempbuf2, 0, INT_MAX,
							&errstr);
						if (errstr != NULL || !find_pid(num)) {
							new_message(MT_standout,
								" %s: unknown pid",
								tempbuf2);
							no_command = true;
						} else {
							if (ps.system == false)
								old_system = false;
							ps.pid = (pid_t)num;
							ps.system = true;
						}
					}
					putchar('\r');
				} else
					clear_message();
				break;
			    default:
				new_message(MT_standout, " BAD CASE IN SWITCH!");
				putchar('\r');
			}
		    }

		    /* flush out stuff that may have been written */
		    fflush(stdout);
		}
	    }
	}
    }

#ifdef DEBUG
    fclose(debug);
#endif
    quit(0);
}

/*
 *  reset_display() - reset all the display routine pointers so that entire
 *	screen will get redrawn.
 */

static void
reset_display(void)
{
    d_loadave    = i_loadave;
    d_procstates = i_procstates;
    d_cpustates  = i_cpustates;
    d_memory     = i_memory;
    d_arc        = i_arc;
    d_carc       = i_carc;
    d_swap       = i_swap;
    d_message	 = i_message;
    d_header	 = i_header;
    d_process	 = i_process;
}

/*
 *  signal handlers
 */

static sigret_t
leave(int i __unused)	/* exit under normal conditions -- INT handler */
{

    leaveflag = 1;
}

static sigret_t
tstop(int i __unused)	/* SIGTSTP handler */
{

    tstopflag = 1;
}

static sigret_t
top_winch(int i __unused)		/* SIGWINCH handler */
{

    winchflag = 1;
}

void
quit(int status)		/* exit under duress */
{
    end_screen();
    exit(status);
}
