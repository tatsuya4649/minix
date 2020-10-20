// signal.h header define all ANSI and POSIX signal.
// minix have all required POSIX signal, and also support some signal.
//
//

#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

// types closely related to signal processing
typedef int sig_atomic_t;

#ifdef _POSIX_SOURCE
#ifndef _SIGSET_T
#define _SIGSET_T
typedef unsigned long sigset_t;
#endif // _SIGSET_T
#endif // _POSIX_SOURCE

#define _NSIG 		16	// number of signals used
#define SIGHUP		1	// hang-up
#define SIGINT		2	// interrupt
#define SIGQUIT 	3	// quit(ASCII FS)
#define SIGILL  	4	// invalid order
#define SIGTRAP 	5	// trace trap (does not reset when acquired)
#define SIGABRT 	6	// IOT order
#define SIGIOT		6	// SIGABRT for people using the PDP-11 format
#define SIGUNUSED 	7	// spare code
#define SIGFPE		8	// floating point exception
#define SIGKILL 	9	// kill (cannot be acquired or ignored)
#define SIGUSR1 	10	// user defined signal
#define SIGSEGV 	11	// segmentation fault
#define SIGUSR2 	12	// user defined signal
#define SIGPIPE 	13	// write to a pipe that no one reads
#define SIGALRM 	14	// alarm clock
#define SIGTERM 	15	// software termination signal from kill

#define SIGEMT		7	// obsolete
#define SIGBUS 		10	// obsolete

// POSIX also has to define unsupported signals, so we define them below.
#define SIGCHKD		17	// child process is finished or stopped 
#define SIGCONT		18	// if stopped, keep gooing
#define SIGSTOP		19	// stop signal
#define SIGTSTP		20	// interactive stop signal
#define SIGTTIN		21	// background process read signal
#define SIGTTOU		22	// background process write signal

// if _POSIX_SOURCE is not defined,can't use sighandler_t type.
#ifdef _POSIX_SOURCE
#define __sighandler_t sighandler_t
#else
typedef void (*__sighandler_t) (int);
#endif // _POSIX_SOURCE

// these macro are user as function pointer
#define SIG_ERR		((__sighandler_t) -1)	// return of error
#define SIG_DFL		((__sighandler_t) 0)	// default signal processing
#define SIG_IGN		((__sighandler_t) 1)	// ignore signal
#define SIG_HOLD	((__sighandler_t) 2)	// block signal
#define SIG_CATCH	((__sighandler_t) 3)	// catch signal

#ifdef _POSIX_SOURCE
struct sigaction{
	__sighandler_t sa_handler;		// pointer pointing to SIG_DFL,SIG_IGN or function
	sigset_t sa_mask;			// blocked signal by handler
	int sa_flags;				// special flags
};

// field of sa_flags(special flags)
#define SA_ONSTACK	0x0001			// send a signal to the alternate stack
#define SA_RESETHAND	0x0002			// reset signal handler when signal is acquired
#define SA_NODEFER	0x0004			// do not block signals while earning
#define SA_RESTART	0x0008			// automatic restart of system calls
#define SA_SIGINFO	0x0010			// extension of signal processing
#define SA_NOCLDWAIT	0x0020			// do not create zombies
#define SA_NOCLDSTOP	0x0040			// do not receive SIGCHLD while the child process is stopped

// POSIX is requiredto use these value with sigprocmask(2)
#define SIG_BLOCK	0			// for block signal
#define SIG_UNBLOCK	1			// for removing block signal
#define SIG_SETMASK	2			// for signla mask setting
#define SIG_INQUIRE	4			// for internal use only
#endif // _POSIX_SOURCE

// POSIX and ANSI function prototype
_PROTOTYPE( int raise,(int _sig));
_PROTOTYPE( __sighandler_t signal, (int _sig,__sighandler_t _func));

#ifdef _POSIX_SOUCE

_PROTOTYPE(int kill,(pid_t _pid,int _sig));
_PROTOTYPE(int sigaction,(int _sig,const struct sigaction* _act,struct sigaction* _oact));
_PROTOTYPE(int sigaddset,(sigset_t* _set,int _sig));
_PROTOTYPE(int sigdelset,(sigset_t* _set,int _sig));
_PROTOTYPE(int sigemptyset,*sigset_t* _set);
_PROTOTYPE(int sigfillset,(sigset_t* _set));
_PROTOTYPE(int sigismember,(sigset_t* _set,int _sig));
_PROTOTYPE(int sigepending,(sigset_t* _set));
_PROTOTYPE(int sigprocmask,(int _how,const sigset_t* _set,sigset_t* _oset));
_PROTOTYPE(int sigsuspend,(const sigset_t* _sigmask));

#endif // _POSIX_SOUCE

#endif // _SIGNAL_H
