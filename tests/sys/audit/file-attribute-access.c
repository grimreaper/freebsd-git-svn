/*-
 * Copyright (c) 2018 Aniket Pandey
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
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <atf-c.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

static struct pollfd fds[1];
static mode_t mode = 0777;
static pid_t pid;
static fhandle_t fht;
static int filedesc, fhdesc;
static char extregex[80];
static struct stat statbuff;
static struct statfs statfsbuff;
static const char *auclass = "fa";
static const char *path = "fileforaudit";
static const char *errpath = "dirdoesnotexist/fileforaudit";
static const char *successreg = "fileforaudit.*return,success";
static const char *failurereg = "fileforaudit.*return,failure";


ATF_TC_WITH_CLEANUP(stat_success);
ATF_TC_HEAD(stat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"stat(2) call");
}

ATF_TC_BODY(stat_success, tc)
{
	/* File needs to exist to call stat(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, stat(path, &statbuff));
	check_audit(fds, successreg, pipefd);
	close(filedesc);
}

ATF_TC_CLEANUP(stat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(stat_failure);
ATF_TC_HEAD(stat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"stat(2) call");
}

ATF_TC_BODY(stat_failure, tc)
{
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, stat(errpath, &statbuff));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(stat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(lstat_success);
ATF_TC_HEAD(lstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"lstat(2) call");
}

ATF_TC_BODY(lstat_success, tc)
{
	/* Symbolic link needs to exist to call lstat(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, lstat(path, &statbuff));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(lstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(lstat_failure);
ATF_TC_HEAD(lstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"lstat(2) call");
}

ATF_TC_BODY(lstat_failure, tc)
{
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, lstat(errpath, &statbuff));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(lstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstat_success);
ATF_TC_HEAD(fstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstat(2) call");
}

ATF_TC_BODY(fstat_success, tc)
{
	/* File needs to exist to call fstat(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT | O_RDWR, mode)) != -1);
	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, fstat(filedesc, &statbuff));

	snprintf(extregex, sizeof(extregex),
		"fstat.*%jd.*return,success", (intmax_t)statbuff.st_ino);
	check_audit(fds, extregex, pipefd);
	close(filedesc);
}

ATF_TC_CLEANUP(fstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstat_failure);
ATF_TC_HEAD(fstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fstat(2) call");
}

ATF_TC_BODY(fstat_failure, tc)
{
	FILE *pipefd = setup(fds, auclass);
	const char *regex = "fstat.*return,failure : Bad file descriptor";
	/* Failure reason: bad file descriptor */
	ATF_REQUIRE_EQ(-1, fstat(-1, &statbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatat_success);
ATF_TC_HEAD(fstatat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstatat(2) call");
}

ATF_TC_BODY(fstatat_success, tc)
{
	/* File or Symbolic link needs to exist to call lstat(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, fstatat(AT_FDCWD, path, &statbuff,
		AT_SYMLINK_NOFOLLOW));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(fstatat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatat_failure);
ATF_TC_HEAD(fstatat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fstatat(2) call");
}

ATF_TC_BODY(fstatat_failure, tc)
{
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, fstatat(AT_FDCWD, path, &statbuff,
		AT_SYMLINK_NOFOLLOW));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(fstatat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(statfs_success);
ATF_TC_HEAD(statfs_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"statfs(2) call");
}

ATF_TC_BODY(statfs_success, tc)
{
	/* File needs to exist to call statfs(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, statfs(path, &statfsbuff));
	check_audit(fds, successreg, pipefd);
	close(filedesc);
}

ATF_TC_CLEANUP(statfs_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(statfs_failure);
ATF_TC_HEAD(statfs_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"statfs(2) call");
}

ATF_TC_BODY(statfs_failure, tc)
{
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, statfs(errpath, &statfsbuff));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(statfs_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatfs_success);
ATF_TC_HEAD(fstatfs_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstatfs(2) call");
}

ATF_TC_BODY(fstatfs_success, tc)
{
	/* File needs to exist to call fstat(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT | O_RDWR, mode)) != -1);
	/* Call stat(2) to store the Inode number of 'path' */
	ATF_REQUIRE_EQ(0, stat(path, &statbuff));
	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, fstatfs(filedesc, &statfsbuff));

	snprintf(extregex, sizeof(extregex), "fstatfs.*%jd.*return,success",
			(intmax_t)statbuff.st_ino);
	check_audit(fds, extregex, pipefd);
	close(filedesc);
}

ATF_TC_CLEANUP(fstatfs_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatfs_failure);
ATF_TC_HEAD(fstatfs_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fstatfs(2) call");
}

ATF_TC_BODY(fstatfs_failure, tc)
{
	FILE *pipefd = setup(fds, auclass);
	const char *regex = "fstatfs.*return,failure : Bad file descriptor";
	/* Failure reason: bad file descriptor */
	ATF_REQUIRE_EQ(-1, fstatfs(-1, &statfsbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fstatfs_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getfsstat_success);
ATF_TC_HEAD(getfsstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"getfsstat(2) call");
}

ATF_TC_BODY(getfsstat_success, tc)
{
	pid = getpid();
	snprintf(extregex, sizeof(extregex), "getfsstat.*%d.*success", pid);

	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE(getfsstat(NULL, 0, MNT_NOWAIT) != -1);
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(getfsstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getfsstat_failure);
ATF_TC_HEAD(getfsstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"getfsstat(2) call");
}

ATF_TC_BODY(getfsstat_failure, tc)
{
	const char *regex = "getfsstat.*return,failure : Invalid argument";
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: Invalid value for mode */
	ATF_REQUIRE_EQ(-1, getfsstat(NULL, 0, -1));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(getfsstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhopen_success);
ATF_TC_HEAD(fhopen_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fhopen(2) call");
}

ATF_TC_BODY(fhopen_success, tc)
{
	pid = getpid();
	snprintf(extregex, sizeof(extregex), "fhopen.*%d.*return,success", pid);

	/* File needs to exist to get a file-handle */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	/* Get the file handle to be passed to fhopen(2) */
	ATF_REQUIRE_EQ(0, getfh(path, &fht));

	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE((fhdesc = fhopen(&fht, O_RDWR)) != -1);
	check_audit(fds, extregex, pipefd);

	close(fhdesc);
	close(filedesc);
}

ATF_TC_CLEANUP(fhopen_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhopen_failure);
ATF_TC_HEAD(fhopen_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fhopen(2) call");
}

ATF_TC_BODY(fhopen_failure, tc)
{
	const char *regex = "fhopen.*return,failure : Invalid argument";
	FILE *pipefd = setup(fds, auclass);
	/*
	 * Failure reason: NULL does not represent any file handle
	 * and O_CREAT is not allowed as the flag for fhopen(2)
	 */
	ATF_REQUIRE_EQ(-1, fhopen(NULL, O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhopen_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstat_success);
ATF_TC_HEAD(fhstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstat(2) call");
}

ATF_TC_BODY(fhstat_success, tc)
{
	pid = getpid();
	snprintf(extregex, sizeof(extregex), "fhstat.*%d.*return,success", pid);

	/* File needs to exist to get a file-handle */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	/* Get the file handle to be passed to fhstat(2) */
	ATF_REQUIRE_EQ(0, getfh(path, &fht));

	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, fhstat(&fht, &statbuff));
	check_audit(fds, extregex, pipefd);
	close(filedesc);
}

ATF_TC_CLEANUP(fhstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstat_failure);
ATF_TC_HEAD(fhstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fhstat(2) call");
}

ATF_TC_BODY(fhstat_failure, tc)
{
	const char *regex = "fhstat.*return,failure : Bad address";
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: NULL does not represent any file handle */
	ATF_REQUIRE_EQ(-1, fhstat(NULL, NULL));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstatfs_success);
ATF_TC_HEAD(fhstatfs_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstatfs(2) call");
}

ATF_TC_BODY(fhstatfs_success, tc)
{
	pid = getpid();
	snprintf(extregex, sizeof(extregex), "fhstatfs.*%d.*success", pid);

	/* File needs to exist to get a file-handle */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	/* Get the file handle to be passed to fhstatfs(2) */
	ATF_REQUIRE_EQ(0, getfh(path, &fht));

	FILE *pipefd = setup(fds, auclass);
	ATF_REQUIRE_EQ(0, fhstatfs(&fht, &statfsbuff));
	check_audit(fds, extregex, pipefd);
	close(filedesc);
}

ATF_TC_CLEANUP(fhstatfs_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstatfs_failure);
ATF_TC_HEAD(fhstatfs_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fhstatfs(2) call");
}

ATF_TC_BODY(fhstatfs_failure, tc)
{
	const char *regex = "fhstatfs.*return,failure : Bad address";
	FILE *pipefd = setup(fds, auclass);
	/* Failure reason: NULL does not represent any file handle */
	ATF_REQUIRE_EQ(-1, fhstatfs(NULL, NULL));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhstatfs_failure, tc)
{
	cleanup();
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, stat_success);
	ATF_TP_ADD_TC(tp, stat_failure);
	ATF_TP_ADD_TC(tp, lstat_success);
	ATF_TP_ADD_TC(tp, lstat_failure);
	ATF_TP_ADD_TC(tp, fstat_success);
	ATF_TP_ADD_TC(tp, fstat_failure);
	ATF_TP_ADD_TC(tp, fstatat_success);
	ATF_TP_ADD_TC(tp, fstatat_failure);

	ATF_TP_ADD_TC(tp, statfs_success);
	ATF_TP_ADD_TC(tp, statfs_failure);
	ATF_TP_ADD_TC(tp, fstatfs_success);
	ATF_TP_ADD_TC(tp, fstatfs_failure);

	ATF_TP_ADD_TC(tp, getfsstat_success);
	ATF_TP_ADD_TC(tp, getfsstat_failure);

	ATF_TP_ADD_TC(tp, fhopen_success);
	ATF_TP_ADD_TC(tp, fhopen_failure);
	ATF_TP_ADD_TC(tp, fhstat_success);
	ATF_TP_ADD_TC(tp, fhstat_failure);
	ATF_TP_ADD_TC(tp, fhstatfs_success);
	ATF_TP_ADD_TC(tp, fhstatfs_failure);

	return (atf_no_error());
}
