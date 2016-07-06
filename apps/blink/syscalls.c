/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>
#include <unistd.h>
#include <sys/wait.h>
#include "cmsis_os.h"

#undef errno
extern int errno;

#undef FreeRTOS
#define MAX_STACK_SIZE 0x200

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

caddr_t _sbrk(int incr)
{
	static char *heap_end;
	extern char heap_low;
	extern char heap_top;
	char *prev_heap_end;

	if (heap_end == 0)
		heap_end = &heap_low;

	prev_heap_end = heap_end;

	if (heap_end + incr > &heap_top)
	{
		write(1, "Heap and stack collision\n", 25);
		//abort();
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_end += incr;

	return (caddr_t) prev_heap_end;
}

/*
 * _gettimeofday primitive (Stub function)
 * */
int _gettimeofday (struct timeval * tp, struct timezone * tzp)
{
  /* Return fixed data for the timezone.  */
  if (tzp)
    {
      tzp->tz_minuteswest = 0;
      tzp->tz_dsttime = 0;
    }

  return 0;
}
void initialise_monitor_handles()
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{
	_kill(status, -1);
	while (1) {}
}

int _read(int file, char *ptr, int len)
{
    int result = 0;
    return 0;
}

int _write(int file, char *data, int len)
{
    return 0;
}

int _close(int file)
{
	return -1;
}

int _fstat(int file, struct stat *st)
{
    if (file != STDOUT_FILENO || file != STDERR_FILENO || file != STDIN_FILENO) {
        errno = EBADF;
        return -1;
    }
    bzero(st, sizeof(stat));
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
    if (file == STDOUT_FILENO || file == STDERR_FILENO || file == STDIN_FILENO) {
        return 1;
    } else {
        errno = EBADF;
        return -1;
    }
}

int _lseek(int file, int ptr, int dir)
{
	return -1;
}

int _open(char *path, int flags, ...)
{
	/* Pretend like we always fail */
	return -1;
}

int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}

int _times(struct tms *buf)
{
	return -1;
}

int _stat(char *file, struct stat *st)
{
    bzero(st, sizeof(stat));
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}
