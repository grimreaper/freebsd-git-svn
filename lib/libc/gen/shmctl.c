#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
int shmctl(int shmid, int cmd, struct shmid_ds *buf)
#else
int shmctl(shmid, cmd, buf)
	int shmid;
	int cmd;
	struct shmid_ds *buf;
#endif
{
	return (shmsys(4, shmid, cmd, buf));
}
