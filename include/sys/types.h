// <sys/types.h> header includes definition of important data types.
// it's a better programming method to use these definitions
// instead of the basic base type.
//
// according to POSIX standard requirements, everything is supposed to end with _t.
//
// defined most of the data types used in minix.
//
//

#ifndef _TYPES_H
#define _TYPES_H

// use _ANSI to check if the compiler is a 16-bit compiler
#ifndef _ANSI
#include <ansi.h>
#endif // _ANSI

// size_t types can save all results of sizeof operator.
// at first glance, it looks like an unsigned int, but
// this is not always the case. 
// for example, if a struct or array size of 70k is required,
// the result is 17bits required for the calculation,
// so size_t must be of type long.
// the type ssize_t is signed size_t.

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif // _SIZE_T

#ifndef _SSIZE_T
#define _SSIZE_T
typedef int ssize_t;
#endif // _SSIZE_T

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;		// elapsed time from GMT (seconds unit)
#endif // _TIME_T

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long clock_t;		// unit of system accounting
#endif // _CLOCK_T

#ifndef _SIGSET_T
#define _SIGSET_T
typedef unsigned long sigset_t;
#endif // _SIGSET_T

// type used in data struct, disk,i node and so on...
typedef short		dev_t;		// own a (major|minor) pair of devices
typedef char 		gid_t;		// group id
typedef unsigned short  ino_t;		// number of i node
typedef unsigned short	mode_t;		// file type and allow bits
typedef char		nlink_t;	// links to file
typedef unsigned long 	off_t;		// offset in file
typedef int 		pid_t;		// process id (signed)
typedef short 		uid_t;		// user id
typedef unsigned long 	zone_t;		// number of zone
typedef unsigned long 	block_t;	// number of block
typedef unsigned long 	bit_t;		// bit number of bitmap
typedef unsigned short 	zone1_t;	// zone number of V1 file system
typedef unsigned short	bitchunk_t;	// a set of bits in a bitmap

typedef unsigned char	u8_t;		// 8 bit type
typedef unsigned short	u16_t;		// 16 bit type
typedef unsigned long	u32_t;		// 32 bit type

typedef char		i8_t;		// 8 bit signed type
typedef short		i16_t;		// 16 bit signed type
typedef long		i32_t;		// 32 bit signed type

// since minix uses K&R type function definitions (to maximize portability),
// the following types are needed: 
// if a short like dev_t is struck by a K&R defined function, 
// the compiler will automatically replace it with int.
// old-fasioned function definitions use int, so prototypes must include int
// as parameters,not shorts. therefore, it is incorrect to use dev_t in the prototype.
// it would be sufficient to use int instead of dev_t in the prototype.
// however, dev_t is easier to understand.
typedef int 		Dev_t;
typedef int 		Gid_t;
typedef int 		Nlink_t;
typedef int 		Uid_t;
typedef int		U8_t;
typedef unsigned int 	U32_t;
typedef int 		I8_t;
typedef int 		I16_t;
typedef long		I32_t;

// writing down unsigned type substitution in ANSI C can be veri tedious.
// If sizeof(short) == sizeof(int),there is no substitution,
// so the type remains unsigned.
// if the compiler is not ANSI,there is usually no loss due to the lack of sign,
// so there is usually no prototype,and the replaced type is not a problem.
// by using a type like Ino_t, we are trying to provide information to the reader
// and use an int (which is not replaced).

#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

#if _EM_WSIZE == 2 || !defined(_ANSI)
typedef unsigned int 	Ino_t;
typedef unsigned int	Zone1_t;
typedef unsigned int	Bitchunk_t;
typedef unsigned int 	U16_t;
typedef unsigned int 	Mode_t;
#else	// _EM_WSIZE == 4 or _EM_WSIZE undefined , or _ANSI defined it
typedef int 		Ino_t;
typedef int		Zone1_t;
typedef int 		Bitchunk_t;
typedef int		U16_t;
typedef int		Mode_t;
#endif	// _EM_WSIZE == 2, etc

// signal handler type such as SIG_IGN
#if defined(_ANSI)
typedef void (*sighandler_t)(int);
#else
typedef void (*sighandler_t)();
#endif // defined(_ANSI)

#endif // _TYPES_H
