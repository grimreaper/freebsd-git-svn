/*
 * The new sysinstall program.
 *
 * This is probably the last program in the `sysinstall' line - the next
 * generation being essentially a complete rewrite.
 *
 * $Id: system.c,v 1.3 1995/05/01 21:56:31 jkh Exp $
 *
 * Jordan Hubbard
 *
 * My contributions are in the public domain.
 *
 * Parts of this file are also blatently stolen from Poul-Henning Kamp's
 * previous version of sysinstall, and as such fall under his "BEERWARE"
 * license, so buy him a beer if you like it!  Buy him a beer for me, too!
 */

#include "sysinstall.h"
#include <signal.h>
#include <sys/reboot.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

/* Handle interrupt signals (duh!) */
static void
handle_intr(int sig)
{
}

/* Welcome the user to the system */
void
systemWelcome(void)
{
}

/* Initialize system defaults */
void
systemInitialize(int argc, char **argv)
{
    signal(SIGINT, SIG_IGN);
    globalsInit();

    /* Are we running as init? */
    if (getpid() == 1) {
	setsid();
	if (argc > 1 && strchr(argv[1],'C')) {
	    /* Kernel told us that we are on a CDROM root */
	    close(0); open("/bootcd/dev/console", O_RDWR);
	    close(1); dup(0);
	    close(2); dup(0);
	    CpioFD = open("/floppies/cpio.flp", O_RDONLY);
	    OnCDROM = TRUE;
	    chroot("/bootcd");
	} else {
	    close(0); open("/dev/console", O_RDWR);
	    close(1); dup(0);
	    close(2); dup(0);
	}
	msgInfo("%s running as init", argv[0]);

	ioctl(0, TIOCSCTTY, (char *)NULL);
	setlogin("root");
	setbuf(stdin, 0);
	setbuf(stdout, 0);
	setbuf(stderr, 0);
    }

    if (set_termcap() == -1)
	msgFatal("Can't find terminal entry");

    /* XXX - libdialog has particularly bad return value checking */
    init_dialog();
    /* If we haven't crashed I guess dialog is running ! */
    DialogActive = TRUE;

    signal(SIGINT, handle_intr);
}

/* Close down and prepare to exit */
void
systemShutdown(void)
{
    if (DialogActive) {
	end_dialog();
	DialogActive = FALSE;
    }
    /* REALLY exit! */
    if (getpid() == 1)
	reboot(RB_HALT);
    else
	exit(1);
}

/* Run some general command */
int
systemExecute(char *command)
{
    int status;

    dialog_clear();
    dialog_update();
    end_dialog();
    DialogActive = FALSE;
    status = system(command);
    DialogActive = TRUE;
    dialog_clear();
    dialog_update();
    return status;
}

/* Find and execute a shell */
int
systemShellEscape(void)
{
    char *sh = NULL;

    if (file_executable("/bin/sh"))
	sh = "/bin/sh";
    else if (file_executable("/stand/sh"))
	sh = "/stand/sh";
    else {
	msgWarn("No shell available, sorry!");
	return 1;
    }
    setenv("PS1", "freebsd% ", 1);
    dialog_clear();
    dialog_update();
    move(0, 0);
    standout();
    addstr("Type `exit' to leave this shell and continue installation");
    standend();
    refresh();
    end_dialog();
    DialogActive = FALSE;
    if (fork() == 0)
	execlp(sh, "-sh", 0);
    else
	wait(NULL);
    dialog_clear();
    DialogActive = TRUE;
    return 0;
}

/* Display a file in a filebox */
int
systemDisplayFile(char *file)
{
    char *fname = NULL;
    char buf[FILENAME_MAX];

    fname = systemHelpFile(file, buf);
    if (!fname) {
	snprintf(buf, FILENAME_MAX, "The %s file is not provided on this particular floppy image.", file);
	use_helpfile(NULL);
	use_helpline(NULL);
	dialog_mesgbox("Sorry!", buf, -1, -1);
	dialog_clear_norefresh();
	return 1;
    }
    else {
	dialog_clear_norefresh();
	use_helpfile(NULL);
	use_helpline(NULL);
	dialog_textbox(file, fname, LINES, COLS);
	dialog_clear_norefresh();
    }
    return 0;
}

char *
systemHelpFile(char *file, char *buf)
{
    char *cp, *fname = NULL;

    if (!file)
	return NULL;

    if ((cp = getenv("LANG")) != NULL) {
	snprintf(buf, FILENAME_MAX, "help/%s/%s", cp, file);
	if (file_readable(buf))
	    fname = buf;
	else {
	    snprintf(buf, FILENAME_MAX, "/stand/help/%s/%s", cp, file);
	    if (file_readable(buf))
		fname = buf;
	}
    }
    else {
	snprintf(buf, FILENAME_MAX, "help/en_US.ISO8859-1/%s", file);
	if (file_readable(buf))
	    fname = buf;
	else {
	    snprintf(buf, FILENAME_MAX, "/stand/help/en_US.ISO8859-1/%s",
		     file);
	    if (file_readable(buf))
		fname = buf;
	}
    }
    return fname;
}

void
systemChangeFont(char *font)
{
}

void
systemChangeLang(char *lang)
{
    variable_set2("LANG", lang);
}

void
systemChangeTerminal(char *color, char *mono)
{
    /* Do something with setterm */
}

void
systemChangeScreenmap(char *newmap)
{
}
