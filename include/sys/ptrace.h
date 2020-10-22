// <sys/ptrace.h> defines various processes that can use
// the PTRACE system call do.
//

#ifndef _PTRACE_H
#define _PTRACE_H

#define T_STOP		-1		// stop process
#define T_OK		0		// enable tracing by the parent process to this process
#define T_GETINS	1		// get a value from instruction space
#define T_GETDATA	2		// get a value from data region
#define T_GETUSER	3		// get a value from user procedd table
#define T_SETINS	4		// set a value to instruction space
#define T_SETDATA	5		// set a value to data space
#define T_SETUSER	6		// set a value to user process table
#define T_RESUME	7		// resume execution
#define T_EXIT		8		// exit
#define T_STEP		9		// set trace bit

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE(long ptrace,(int _req,pid_t _pid,long _addr,long _data));

#endif // _PTRACE_H
