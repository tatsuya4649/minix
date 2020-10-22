// <sys/wait.h> header includes macros about wait().
// the return value of wait() and waitpid() are ddetermined by
// whether the process was terminated by the exit() call,
// killed by a signal,or stopped by job control,as follow.
//
//
//				upper byte 	lower byte
//				+-------------------------------+
//	exit(status)		|     status	|       0	|
//				+-------------------------------+
//	killed by a signal	|	0	|     signal	|
//				+-------------------------------+
//	stop(job control)	|     signal	|      0177	|
//				+-------------------------------+
//
#ifndef _WAIT_H
#define _WAIT_H

#define _LOW(v)			((v)&0377)
#define _HIGH(v)		(((v)>>8)&0377)

#define WNOHANG			1		// don't wait for the child process to finish
#define WUNTRACED		2		// for job control : not implemention

#define WIFEXITED(s)		(_LOW(s) == 0)				// exit normal
#define WEXITSTATUS(s)		(_HIGH(s))				// exit status
#define WTERMSIG(s)		(_LOW(s) & 0177)			// signal value
#define WIFSIGNALED(s)		(((unsigned int)(s)-1 & 0xFFFF) < 0xFF)	// signal generation
#define WIFSTOPPED(s)		(_LOW(s) == 0177)			// stop
#define WSTOPSIG(s)		(_HIGH(s) & 0377)			// stop signal

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE(pid_t wait,(int *_stat_loc));
_PROTOTYPE(pid_t waitpid,(pid_t _pid,int *_stat_loc,int _options));

#endif // _WAIT_H
