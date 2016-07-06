/*
 * sys.c
 *
 *  Created on: Jul 5, 2016
 *      Author: jmiller
 */
#include <errno.h>

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
