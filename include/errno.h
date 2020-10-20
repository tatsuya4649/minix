// errno.h is to define the number of various errors that can occur during program excution.
// these error number is small positive integer, and is saw from user program.
// the following mechanism is used to solve the problem that the error number is negative inside the system and positive outside.
// All definitions are made in the following format.
//
//	#define EPERM	(_SIGN 1)
//
// if _SYSTEM is defined,_SIGN define "-". in other case, _SIGN define "".
// when you compile operating system , _SYSTEM is defined,and set EPERM(-1).
// in other hand,when this file contains in normal user program,set EPRTM(1).
//
//

#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef _SYSTEM
#	define _SIGN -
#	define OK 0
#else
#	define _SIGN
#endif

extern int errno; // error number setting destination

// the following the numerical value of the error number
#define EGENERIC (_SIGN 90)	// generic error
#define EPERM (_SIGN 1)		// operation is not allowed
#define ENOENT (_SIGN 2)	// missing specified file of delay
#define ESRCH (_SIGN 3)		// missing process
#define EINTR (_SIGN 4)		// function call interrupt
#define EIO (_SIGN 5)		// I/O error
#define ENXIO (_SIGN 6)		// the specified devide or addredd cannot be found
#define E2BIG (_SIGN 7)		// too long arguments list
#define ENOEXEC (_SIGN 8)	// exec format error
#define EBADF (_SIGN 9)		// file desctiptor eror
#define ECHILD (_SIGN 10)	// no child process
#define EAGAIN (_SIGN 11)	// resources are temporarily unavailable
#define ENOMEM (_SIGN 12)	// lack of space
#define EACCES (_SIGN 13)	// not allow
#define EFAULT (_SIGN 14)	// wrong address
#define ENOTBLK (_SIGN 15)	// expand: not block special file
#define EBUSY (_SIGN 16)	// resource is used now
#define EEXIST (_SIGN 17)	// file already exsists
#define EXDEV	(_SIGN 18)	// inappropriate ling
#define ENODEV (_SIGN 19)	// there is no specified device
#define ENOTDIR	(_SIGN 20)	// is not directories
#define EISDIR (_SIGN 21)	// is directories
#define EINVAL (_SIGN 22)	// invalid argument
#define ENFILE (_SIGN 23)	// too many opened file in system
#define EMFILE (_SIGN 24)	// too many opened file
#define ENOTTY (_SIGN 25)	// inappropriate I/O control operation
#define ETXTBSY (_SIGN 26)	// is no longer used
#define EFBIG (_SIGN 27)	// file is too big
#define ENOSPC (_SIGN 28)	// device don't have space
#define ESPIPE (_SIGN 29)	// invalid seek
#define EROFS (_SIGN 30)	// only read file system
#define EMLINK (_SIGN 31)	// too many links
#define EPIPE (_SIGN 32)	// broken pipe
#define EDOM (_SIGN 33)		// domain error(ANSI C)
#define ERANGE (_SIGN 34)	// result is too big(ANSI C)
#define EDEADLK (_SIGN 35)	// avoided resource deadlock
#define ENAMETOOLONG (_SIGN 36)	// file name is too long
#define ENOLCK (_SIGN 37)	// can't lock
#define ENOSYS (_SIGN 38)	// function is not implemented
#define ENOTEMPTY (_SIGN 39)	// the delay directory is not free
// following error is about "network"
#define EPACKSIZE (_SIGN 50)	// invalid packet size for some protocols
#define EOUTOFBUFS (_SIGN 51)	// no enough buffer
#define EBADIOCTL (_SIGN 52)	// invalid ioctl in device
#define EBADMODE (_SIGN 53)	// illegal mode ioctl
#define EWOULDBLOCK (_SIGN 54)
#define EVADDEST (_SIGN 55)	// invalid destination address
#define EDSTNOTRCH (_SIGN 56)	// unable to reach the destination
#define EISCONN (_SIGN 57)	// already all connected
#define EADDRINUSE (_SIGN 58)	// address in use
#define ECONNREFUSED (_SIGN 59)	// connection refused
#define ECONNREST (_SIGN 60)	// connection reset
#define ETIMEDOUT (_SIGN 61)	// connection time out
#define EURG (_SIGN 62)		// urgent data exists
#define ENOURG (_SIGN 63)	// urgent data not exists
#define ENOTCONN (_SIGN 64)	// not connection
#define ESHUTDONW (_SIGN 65)	// attempted to write to a broken connection
#define ENOCONN (_SIGN 66)	// specified connection not exists

//following not POSIX error,but possible to occur
#define ELOCKED (_SIGN 101)	// can't send message
#define EBADCALL (_SIGN 102)	// send and receive error

// the following error code is also generated from kernel itself
#ifdef _SYSTEM
#define E_BAD_DEST -1001 	// invalid destination address
#define E_BAD_SRC -1002		// invalid souce address
#define E_TRY_AGAIN -1003	// can't send -- filled table
#define E_OVERRUN -1004		// interrupts for tasks that are not waiting
#define E_BAD_BUF -1005		// message buffer outside the caller's address area
#define E_TASK -1006		// can't send to task
#define E_NO_MESSAGE -1007	// failure RECIEVE : no message
#define E_NO_PERM -1008		// normal users cannot send to tasks
#define E_BAD_FCN -1009		// functions other than SEND,RECIEVE,BOTH are invalid
#define E_BAD_ADDR -1010	// illegal address passed to utility routine
#define E_BAD_PROC -1011	// illegal process number passed to utility routine

#endif // _SYSTEM

#endif // _ERRNO_H
