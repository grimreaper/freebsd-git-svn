#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/libkern.h>

#if defined(__SSP__) || defined(__SSP_ALL__)
long __stack_chk_guard[8] = {};
void __stack_chk_fail(void);

void
__stack_chk_fail(void)
{

	panic("stack overflow detected; backtrace may be corrupted");
}

#define __arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))
static void
__stack_chk_init(void *dummy __unused)
{
	size_t i;
	long guard[__arraycount(__stack_chk_guard)];

	arc4rand(guard, sizeof(guard), 0);
	for (i = 0; i < __arraycount(guard); i++)
		__stack_chk_guard[i] = guard[i];
}
/* SI_SUB_EVENTHANDLER is right after SI_SUB_LOCK used by arc4rand() init. */
SYSINIT(stack_chk, SI_SUB_EVENTHANDLER, SI_ORDER_ANY, __stack_chk_init, NULL);

#endif
