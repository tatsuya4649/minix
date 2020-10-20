// limits.h header is to define basic size of "language type" and "operating system type".
//

#ifndef _LIMITS_H
#define _LIMITS_H

// define "char" (8-bit(1-byte) and signed on minix)
#define CHAR_BIT 8 	// bit size of char
#define CHAR_MIN -128	// size of char(signed) min
#define CHAR_MAX 127	// size of char(signed) max
#define SCHAR_MIN -128  // size of signed char min
#define SCHAR_MAX 127	// size of signed char max
#define UCHAR_MAX 255	// size of unsigned char max
#define MB_LEN_MAX 1	// lengh of multiple bytes of char

// define "short" (16-bit on minix)
#define SHRT_MIN (-32767-1)	// size of short min
#define SHRT_MAX 32767		// size of short max
#define USHRT_MAX 0xFFFF	// size of unsigned short max

// _EM_WSIZE is a compiler-generated symbol giving the word size in bytes
#if _EM_WSIZE == 2
#define INT_MIN (-32767-1)	// size of int min
#define INT_MAX 32767		// size of int max
#define UINT_MAX 0xFFFF		// size of unsigned max
#endif

#if _EM_WSIZE == 4
#define INT_MIN (-2147483647-1)	// size of int min
#define INT_MAX 2147483647	// size of int max
#define UINT_MAX 0xFFFFFFFF	// size of unsigned max

// define "long"
#define LONG_MIN (-2147483647-1)	// size of long min
#define LONG_MAX 2147483647		// size of long max
#define ULONG_MAX 0xFFFFFFFF		// size of unsigned long

// minimum size required by POSIX
#ifdef _POSIX_SOURCE // this is used only POSIX
#define _POSIX_ARG_MAX 4096	// exec() must have 4k args
#define _POSIX_CHILD_MAX 6	// 1 process have 6 child process
#define _POSIX_LINK_MAX 8	// 1 file have 8 links
#define _POSIX_MAX_CANON 255	// size of standard input que 
#define _POSIX_MAX_INPUT 255	// pre-enter 255 characters
#define _POSIX_NAME_MAX 14	// maximum file name length
#define _POSIX_NGROUPS_MAX 0	// capture group ID is optional
#define _POSIX_OPEN_MAX 16	// 1 process open max 16 files
#define _POSIX_PATH_MAX 255	// path name maximum length is 255
#define _POSIX_PIPE_BUF 512	// pip writing is done in 512 bytes each
#define _POSIX_STREAM_MAX 8	// must be able to open at least 8 files at the same time
#define _POSIX_TZNAME_MAX 3 	// time zone name can be at least 3 characters
#define _POSIX_SSIZE_MAX 32767	// read() support reading 32767 bites

// the value that minix actually implements
// for non_POSIX,it's better to define such an old name
#define _NO_LIMIT 100		// arbitraty number ; no limit;
#define NGROUPS_MAX 0		// ca[tire gtoup ID is optional
#if _EM_WSIZE > 2
#define ARG_MAX 16384		// arguments for exec() and number of bytes in the environment
#else
#define ARG_MAX 4096
#endif // _EM_WSIZE > 2

#define CHILD_MAX _NO_LIMIT	// minix does not limit the number of child process
#define OPEN_MAX 20		// 1 process open max 20 files
#define LINK_MAX 127		// 1 file have max 127 links
#define MAX_CANON 255		// size of standard input que
#define MAX_INPUT 255		// pre-enter 255 characters
#define NAME_MAX 14		// maximum file name lenght is 14
#define PATH_MAX 255		// path name maximum lenght is 255
#define PIPE_BUF 7186		// pip writing is done in 7186 bytes each
#define STREAM_MAX 20		// must be the same as FOPEN_MAX in stdio.h
#define TZNAME_MAX 3		// time zone name can be at least 3 characters
#define SSIZE_MAX 32767		// maximum bytes count defined for read()

#endif // _POSIX_SOURCE

#endif // _LIMITS_H
