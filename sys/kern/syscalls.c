/*
 * System call names.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * $FreeBSD$
 * created from FreeBSD: src/sys/kern/syscalls.master,v 1.126 2002/10/09 21:47:04 rwatson Exp 
 */

char *syscallnames[] = {
	"syscall",			/* 0 = syscall */
	"exit",			/* 1 = exit */
	"fork",			/* 2 = fork */
	"read",			/* 3 = read */
	"write",			/* 4 = write */
	"open",			/* 5 = open */
	"close",			/* 6 = close */
	"wait4",			/* 7 = wait4 */
	"old.creat",		/* 8 = old creat */
	"link",			/* 9 = link */
	"unlink",			/* 10 = unlink */
	"obs_execv",			/* 11 = obsolete execv */
	"chdir",			/* 12 = chdir */
	"fchdir",			/* 13 = fchdir */
	"mknod",			/* 14 = mknod */
	"chmod",			/* 15 = chmod */
	"chown",			/* 16 = chown */
	"break",			/* 17 = break */
	"getfsstat",			/* 18 = getfsstat */
	"old.lseek",		/* 19 = old lseek */
	"getpid",			/* 20 = getpid */
	"mount",			/* 21 = mount */
	"unmount",			/* 22 = unmount */
	"setuid",			/* 23 = setuid */
	"getuid",			/* 24 = getuid */
	"geteuid",			/* 25 = geteuid */
	"ptrace",			/* 26 = ptrace */
	"recvmsg",			/* 27 = recvmsg */
	"sendmsg",			/* 28 = sendmsg */
	"recvfrom",			/* 29 = recvfrom */
	"accept",			/* 30 = accept */
	"getpeername",			/* 31 = getpeername */
	"getsockname",			/* 32 = getsockname */
	"access",			/* 33 = access */
	"chflags",			/* 34 = chflags */
	"fchflags",			/* 35 = fchflags */
	"sync",			/* 36 = sync */
	"kill",			/* 37 = kill */
	"old.stat",		/* 38 = old stat */
	"getppid",			/* 39 = getppid */
	"old.lstat",		/* 40 = old lstat */
	"dup",			/* 41 = dup */
	"pipe",			/* 42 = pipe */
	"getegid",			/* 43 = getegid */
	"profil",			/* 44 = profil */
	"ktrace",			/* 45 = ktrace */
	"old.sigaction",		/* 46 = old sigaction */
	"getgid",			/* 47 = getgid */
	"old.sigprocmask",		/* 48 = old sigprocmask */
	"getlogin",			/* 49 = getlogin */
	"setlogin",			/* 50 = setlogin */
	"acct",			/* 51 = acct */
	"old.sigpending",		/* 52 = old sigpending */
	"sigaltstack",			/* 53 = sigaltstack */
	"ioctl",			/* 54 = ioctl */
	"reboot",			/* 55 = reboot */
	"revoke",			/* 56 = revoke */
	"symlink",			/* 57 = symlink */
	"readlink",			/* 58 = readlink */
	"execve",			/* 59 = execve */
	"umask",			/* 60 = umask */
	"chroot",			/* 61 = chroot */
	"old.fstat",		/* 62 = old fstat */
	"old.getkerninfo",		/* 63 = old getkerninfo */
	"old.getpagesize",		/* 64 = old getpagesize */
	"msync",			/* 65 = msync */
	"vfork",			/* 66 = vfork */
	"obs_vread",			/* 67 = obsolete vread */
	"obs_vwrite",			/* 68 = obsolete vwrite */
	"sbrk",			/* 69 = sbrk */
	"sstk",			/* 70 = sstk */
	"old.mmap",		/* 71 = old mmap */
	"vadvise",			/* 72 = vadvise */
	"munmap",			/* 73 = munmap */
	"mprotect",			/* 74 = mprotect */
	"madvise",			/* 75 = madvise */
	"obs_vhangup",			/* 76 = obsolete vhangup */
	"obs_vlimit",			/* 77 = obsolete vlimit */
	"mincore",			/* 78 = mincore */
	"getgroups",			/* 79 = getgroups */
	"setgroups",			/* 80 = setgroups */
	"getpgrp",			/* 81 = getpgrp */
	"setpgid",			/* 82 = setpgid */
	"setitimer",			/* 83 = setitimer */
	"old.wait",		/* 84 = old wait */
	"swapon",			/* 85 = swapon */
	"getitimer",			/* 86 = getitimer */
	"old.gethostname",		/* 87 = old gethostname */
	"old.sethostname",		/* 88 = old sethostname */
	"getdtablesize",			/* 89 = getdtablesize */
	"dup2",			/* 90 = dup2 */
	"#91",			/* 91 = getdopt */
	"fcntl",			/* 92 = fcntl */
	"select",			/* 93 = select */
	"#94",			/* 94 = setdopt */
	"fsync",			/* 95 = fsync */
	"setpriority",			/* 96 = setpriority */
	"socket",			/* 97 = socket */
	"connect",			/* 98 = connect */
	"old.accept",		/* 99 = old accept */
	"getpriority",			/* 100 = getpriority */
	"old.send",		/* 101 = old send */
	"old.recv",		/* 102 = old recv */
	"osigreturn",			/* 103 = osigreturn */
	"bind",			/* 104 = bind */
	"setsockopt",			/* 105 = setsockopt */
	"listen",			/* 106 = listen */
	"obs_vtimes",			/* 107 = obsolete vtimes */
	"old.sigvec",		/* 108 = old sigvec */
	"old.sigblock",		/* 109 = old sigblock */
	"old.sigsetmask",		/* 110 = old sigsetmask */
	"old.sigsuspend",		/* 111 = old sigsuspend */
	"old.sigstack",		/* 112 = old sigstack */
	"old.recvmsg",		/* 113 = old recvmsg */
	"old.sendmsg",		/* 114 = old sendmsg */
	"obs_vtrace",			/* 115 = obsolete vtrace */
	"gettimeofday",			/* 116 = gettimeofday */
	"getrusage",			/* 117 = getrusage */
	"getsockopt",			/* 118 = getsockopt */
	"#119",			/* 119 = resuba */
	"readv",			/* 120 = readv */
	"writev",			/* 121 = writev */
	"settimeofday",			/* 122 = settimeofday */
	"fchown",			/* 123 = fchown */
	"fchmod",			/* 124 = fchmod */
	"old.recvfrom",		/* 125 = old recvfrom */
	"setreuid",			/* 126 = setreuid */
	"setregid",			/* 127 = setregid */
	"rename",			/* 128 = rename */
	"old.truncate",		/* 129 = old truncate */
	"old.ftruncate",		/* 130 = old ftruncate */
	"flock",			/* 131 = flock */
	"mkfifo",			/* 132 = mkfifo */
	"sendto",			/* 133 = sendto */
	"shutdown",			/* 134 = shutdown */
	"socketpair",			/* 135 = socketpair */
	"mkdir",			/* 136 = mkdir */
	"rmdir",			/* 137 = rmdir */
	"utimes",			/* 138 = utimes */
	"obs_4.2",			/* 139 = obsolete 4.2 sigreturn */
	"adjtime",			/* 140 = adjtime */
	"old.getpeername",		/* 141 = old getpeername */
	"old.gethostid",		/* 142 = old gethostid */
	"old.sethostid",		/* 143 = old sethostid */
	"old.getrlimit",		/* 144 = old getrlimit */
	"old.setrlimit",		/* 145 = old setrlimit */
	"old.killpg",		/* 146 = old killpg */
	"setsid",			/* 147 = setsid */
	"quotactl",			/* 148 = quotactl */
	"old.quota",		/* 149 = old quota */
	"old.getsockname",		/* 150 = old getsockname */
	"#151",			/* 151 = sem_lock */
	"#152",			/* 152 = sem_wakeup */
	"#153",			/* 153 = asyncdaemon */
	"#154",			/* 154 = nosys */
	"nfssvc",			/* 155 = nfssvc */
	"old.getdirentries",		/* 156 = old getdirentries */
	"statfs",			/* 157 = statfs */
	"fstatfs",			/* 158 = fstatfs */
	"#159",			/* 159 = nosys */
	"#160",			/* 160 = nosys */
	"getfh",			/* 161 = getfh */
	"getdomainname",			/* 162 = getdomainname */
	"setdomainname",			/* 163 = setdomainname */
	"uname",			/* 164 = uname */
	"sysarch",			/* 165 = sysarch */
	"rtprio",			/* 166 = rtprio */
	"#167",			/* 167 = nosys */
	"#168",			/* 168 = nosys */
	"semsys",			/* 169 = semsys */
	"msgsys",			/* 170 = msgsys */
	"shmsys",			/* 171 = shmsys */
	"#172",			/* 172 = nosys */
	"pread",			/* 173 = pread */
	"pwrite",			/* 174 = pwrite */
	"#175",			/* 175 = nosys */
	"ntp_adjtime",			/* 176 = ntp_adjtime */
	"#177",			/* 177 = sfork */
	"#178",			/* 178 = getdescriptor */
	"#179",			/* 179 = setdescriptor */
	"#180",			/* 180 = nosys */
	"setgid",			/* 181 = setgid */
	"setegid",			/* 182 = setegid */
	"seteuid",			/* 183 = seteuid */
	"#184",			/* 184 = lfs_bmapv */
	"#185",			/* 185 = lfs_markv */
	"#186",			/* 186 = lfs_segclean */
	"#187",			/* 187 = lfs_segwait */
	"stat",			/* 188 = stat */
	"fstat",			/* 189 = fstat */
	"lstat",			/* 190 = lstat */
	"pathconf",			/* 191 = pathconf */
	"fpathconf",			/* 192 = fpathconf */
	"#193",			/* 193 = nosys */
	"getrlimit",			/* 194 = getrlimit */
	"setrlimit",			/* 195 = setrlimit */
	"getdirentries",			/* 196 = getdirentries */
	"mmap",			/* 197 = mmap */
	"__syscall",			/* 198 = __syscall */
	"lseek",			/* 199 = lseek */
	"truncate",			/* 200 = truncate */
	"ftruncate",			/* 201 = ftruncate */
	"__sysctl",			/* 202 = __sysctl */
	"mlock",			/* 203 = mlock */
	"munlock",			/* 204 = munlock */
	"undelete",			/* 205 = undelete */
	"futimes",			/* 206 = futimes */
	"getpgid",			/* 207 = getpgid */
	"#208",			/* 208 = newreboot */
	"poll",			/* 209 = poll */
	"lkmnosys",			/* 210 = lkmnosys */
	"lkmnosys",			/* 211 = lkmnosys */
	"lkmnosys",			/* 212 = lkmnosys */
	"lkmnosys",			/* 213 = lkmnosys */
	"lkmnosys",			/* 214 = lkmnosys */
	"lkmnosys",			/* 215 = lkmnosys */
	"lkmnosys",			/* 216 = lkmnosys */
	"lkmnosys",			/* 217 = lkmnosys */
	"lkmnosys",			/* 218 = lkmnosys */
	"lkmnosys",			/* 219 = lkmnosys */
	"__semctl",			/* 220 = __semctl */
	"semget",			/* 221 = semget */
	"semop",			/* 222 = semop */
	"#223",			/* 223 = semconfig */
	"msgctl",			/* 224 = msgctl */
	"msgget",			/* 225 = msgget */
	"msgsnd",			/* 226 = msgsnd */
	"msgrcv",			/* 227 = msgrcv */
	"shmat",			/* 228 = shmat */
	"shmctl",			/* 229 = shmctl */
	"shmdt",			/* 230 = shmdt */
	"shmget",			/* 231 = shmget */
	"clock_gettime",			/* 232 = clock_gettime */
	"clock_settime",			/* 233 = clock_settime */
	"clock_getres",			/* 234 = clock_getres */
	"#235",			/* 235 = timer_create */
	"#236",			/* 236 = timer_delete */
	"#237",			/* 237 = timer_settime */
	"#238",			/* 238 = timer_gettime */
	"#239",			/* 239 = timer_getoverrun */
	"nanosleep",			/* 240 = nanosleep */
	"#241",			/* 241 = nosys */
	"#242",			/* 242 = nosys */
	"#243",			/* 243 = nosys */
	"#244",			/* 244 = nosys */
	"#245",			/* 245 = nosys */
	"#246",			/* 246 = nosys */
	"#247",			/* 247 = nosys */
	"#248",			/* 248 = nosys */
	"#249",			/* 249 = nosys */
	"minherit",			/* 250 = minherit */
	"rfork",			/* 251 = rfork */
	"openbsd_poll",			/* 252 = openbsd_poll */
	"issetugid",			/* 253 = issetugid */
	"lchown",			/* 254 = lchown */
	"#255",			/* 255 = nosys */
	"#256",			/* 256 = nosys */
	"#257",			/* 257 = nosys */
	"#258",			/* 258 = nosys */
	"#259",			/* 259 = nosys */
	"#260",			/* 260 = nosys */
	"#261",			/* 261 = nosys */
	"#262",			/* 262 = nosys */
	"#263",			/* 263 = nosys */
	"#264",			/* 264 = nosys */
	"#265",			/* 265 = nosys */
	"#266",			/* 266 = nosys */
	"#267",			/* 267 = nosys */
	"#268",			/* 268 = nosys */
	"#269",			/* 269 = nosys */
	"#270",			/* 270 = nosys */
	"#271",			/* 271 = nosys */
	"getdents",			/* 272 = getdents */
	"#273",			/* 273 = nosys */
	"lchmod",			/* 274 = lchmod */
	"netbsd_lchown",			/* 275 = netbsd_lchown */
	"lutimes",			/* 276 = lutimes */
	"netbsd_msync",			/* 277 = netbsd_msync */
	"nstat",			/* 278 = nstat */
	"nfstat",			/* 279 = nfstat */
	"nlstat",			/* 280 = nlstat */
	"#281",			/* 281 = nosys */
	"#282",			/* 282 = nosys */
	"#283",			/* 283 = nosys */
	"#284",			/* 284 = nosys */
	"#285",			/* 285 = nosys */
	"#286",			/* 286 = nosys */
	"#287",			/* 287 = nosys */
	"#288",			/* 288 = nosys */
	"#289",			/* 289 = nosys */
	"#290",			/* 290 = nosys */
	"#291",			/* 291 = nosys */
	"#292",			/* 292 = nosys */
	"#293",			/* 293 = nosys */
	"#294",			/* 294 = nosys */
	"#295",			/* 295 = nosys */
	"#296",			/* 296 = nosys */
	"fhstatfs",			/* 297 = fhstatfs */
	"fhopen",			/* 298 = fhopen */
	"fhstat",			/* 299 = fhstat */
	"modnext",			/* 300 = modnext */
	"modstat",			/* 301 = modstat */
	"modfnext",			/* 302 = modfnext */
	"modfind",			/* 303 = modfind */
	"kldload",			/* 304 = kldload */
	"kldunload",			/* 305 = kldunload */
	"kldfind",			/* 306 = kldfind */
	"kldnext",			/* 307 = kldnext */
	"kldstat",			/* 308 = kldstat */
	"kldfirstmod",			/* 309 = kldfirstmod */
	"getsid",			/* 310 = getsid */
	"setresuid",			/* 311 = setresuid */
	"setresgid",			/* 312 = setresgid */
	"obs_signanosleep",			/* 313 = obsolete signanosleep */
	"aio_return",			/* 314 = aio_return */
	"aio_suspend",			/* 315 = aio_suspend */
	"aio_cancel",			/* 316 = aio_cancel */
	"aio_error",			/* 317 = aio_error */
	"aio_read",			/* 318 = aio_read */
	"aio_write",			/* 319 = aio_write */
	"lio_listio",			/* 320 = lio_listio */
	"yield",			/* 321 = yield */
	"obs_thr_sleep",			/* 322 = obsolete thr_sleep */
	"obs_thr_wakeup",			/* 323 = obsolete thr_wakeup */
	"mlockall",			/* 324 = mlockall */
	"munlockall",			/* 325 = munlockall */
	"__getcwd",			/* 326 = __getcwd */
	"sched_setparam",			/* 327 = sched_setparam */
	"sched_getparam",			/* 328 = sched_getparam */
	"sched_setscheduler",			/* 329 = sched_setscheduler */
	"sched_getscheduler",			/* 330 = sched_getscheduler */
	"sched_yield",			/* 331 = sched_yield */
	"sched_get_priority_max",			/* 332 = sched_get_priority_max */
	"sched_get_priority_min",			/* 333 = sched_get_priority_min */
	"sched_rr_get_interval",			/* 334 = sched_rr_get_interval */
	"utrace",			/* 335 = utrace */
	"old.sendfile",		/* 336 = old sendfile */
	"kldsym",			/* 337 = kldsym */
	"jail",			/* 338 = jail */
	"#339",			/* 339 = pioctl */
	"sigprocmask",			/* 340 = sigprocmask */
	"sigsuspend",			/* 341 = sigsuspend */
	"sigaction",			/* 342 = sigaction */
	"sigpending",			/* 343 = sigpending */
	"sigreturn",			/* 344 = sigreturn */
	"#345",			/* 345 = sigtimedwait */
	"#346",			/* 346 = sigwaitinfo */
	"__acl_get_file",			/* 347 = __acl_get_file */
	"__acl_set_file",			/* 348 = __acl_set_file */
	"__acl_get_fd",			/* 349 = __acl_get_fd */
	"__acl_set_fd",			/* 350 = __acl_set_fd */
	"__acl_delete_file",			/* 351 = __acl_delete_file */
	"__acl_delete_fd",			/* 352 = __acl_delete_fd */
	"__acl_aclcheck_file",			/* 353 = __acl_aclcheck_file */
	"__acl_aclcheck_fd",			/* 354 = __acl_aclcheck_fd */
	"extattrctl",			/* 355 = extattrctl */
	"extattr_set_file",			/* 356 = extattr_set_file */
	"extattr_get_file",			/* 357 = extattr_get_file */
	"extattr_delete_file",			/* 358 = extattr_delete_file */
	"aio_waitcomplete",			/* 359 = aio_waitcomplete */
	"getresuid",			/* 360 = getresuid */
	"getresgid",			/* 361 = getresgid */
	"kqueue",			/* 362 = kqueue */
	"kevent",			/* 363 = kevent */
	"#364",			/* 364 = __cap_get_proc */
	"#365",			/* 365 = __cap_set_proc */
	"#366",			/* 366 = __cap_get_fd */
	"#367",			/* 367 = __cap_get_file */
	"#368",			/* 368 = __cap_set_fd */
	"#369",			/* 369 = __cap_set_file */
	"lkmressys",			/* 370 = lkmressys */
	"extattr_set_fd",			/* 371 = extattr_set_fd */
	"extattr_get_fd",			/* 372 = extattr_get_fd */
	"extattr_delete_fd",			/* 373 = extattr_delete_fd */
	"__setugid",			/* 374 = __setugid */
	"nfsclnt",			/* 375 = nfsclnt */
	"eaccess",			/* 376 = eaccess */
	"#377",			/* 377 = afs_syscall */
	"nmount",			/* 378 = nmount */
	"kse_exit",			/* 379 = kse_exit */
	"kse_wakeup",			/* 380 = kse_wakeup */
	"kse_create",			/* 381 = kse_create */
	"kse_thr_interrupt",			/* 382 = kse_thr_interrupt */
	"kse_release",			/* 383 = kse_release */
	"__mac_get_proc",			/* 384 = __mac_get_proc */
	"__mac_set_proc",			/* 385 = __mac_set_proc */
	"__mac_get_fd",			/* 386 = __mac_get_fd */
	"__mac_get_file",			/* 387 = __mac_get_file */
	"__mac_set_fd",			/* 388 = __mac_set_fd */
	"__mac_set_file",			/* 389 = __mac_set_file */
	"kenv",			/* 390 = kenv */
	"lchflags",			/* 391 = lchflags */
	"uuidgen",			/* 392 = uuidgen */
	"sendfile",			/* 393 = sendfile */
	"mac_syscall",			/* 394 = mac_syscall */
	"#395",			/* 395 = nosys */
	"#396",			/* 396 = nosys */
	"#397",			/* 397 = nosys */
	"#398",			/* 398 = nosys */
	"#399",			/* 399 = nosys */
	"ksem_close",			/* 400 = ksem_close */
	"ksem_post",			/* 401 = ksem_post */
	"ksem_wait",			/* 402 = ksem_wait */
	"ksem_trywait",			/* 403 = ksem_trywait */
	"ksem_init",			/* 404 = ksem_init */
	"ksem_open",			/* 405 = ksem_open */
	"ksem_unlink",			/* 406 = ksem_unlink */
	"ksem_getvalue",			/* 407 = ksem_getvalue */
	"ksem_destroy",			/* 408 = ksem_destroy */
	"#409",			/* 409 = __mac_get_pid */
	"#410",			/* 410 = __mac_get_link */
	"#411",			/* 411 = __mac_set_link */
	"extattr_set_link",			/* 412 = extattr_set_link */
	"extattr_get_link",			/* 413 = extattr_get_link */
	"extattr_delete_link",			/* 414 = extattr_delete_link */
};
