// fcntl.h header is requires open() and fcntl() system calls
// with various parameters and flags
// the format of the call is
//
//	open(path, oflag[, mode]) => open file
//	fcntl(fd, cmd[, arg]) => get or set file attributes
//	
//

#ifndef _FCNTL_H
#define _FCNTL_H

// following value is used by fcntl() command
#define F_DUPFD			0 	// copy file descriptor
#define F_GETFD			1 	// get file descriptor flag
#define F_SETFD			2 	// set file descriptor flag
#define F_GETFL			3 	// get file status flag
#define F_SETFL			4 	// set file status flag
#define F_GETLK			5 	// get record lock information
#define F_SETLK			6 	// set record lock information
#define F_SETLKW		7 	// set record lock information: if blocked, wait

// file desctiptor flag is used by fcntl()
#define FD_CLOEXEC		1 	// exec close flag,fcntl the third argument

// L_type value of record lock in fcntl()
#define F_RDLCK			1	// shared or read lock
#define F_WELCK			2	// exclusion or write lock
#define F_UNLCK			3	// unlock

// Oflags value in open()
#define O_CREAT			00100	// if not exists,create file
#define O_EXCL			00200	// exclusive use flag
#define O_NOCTTY		00400	// do not assign control terminals
#define O_TRUNC			01000	// truncate the file

// file status flag in open() and fcntl()
#define O_APPEND		02000	// set append mode
#define O_NONBLOCK		04000	// no delay

// file access mode in open() and fcntl()
#define O_RDONLY		0	// open(name,O_RDONLY):open only read
#define O_WRONLY		1	// open(name,O_WEONLY):open only write
#define O_RDWR			2	// open(name,O_RDWR):open read and write

// mask used in file access mode
#define O_ACCMODE		03	// mask of file access mode

// struct user for locking
struct flock{
	short l_type; 	// type:F_RDLCK,F_WRLCK,F_UNLCK
	short l_whence;	// offset start flag
	off_t l_start;	// relative offset (number of bytes)
	off_t l_len;	// size: if 0, until EOF
	pid_t l_pid;	// the process ID of the lock owner
};

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif

_PROTOTYPE(int create,(const char* _path,Mode_t _mode));
_PROTOTYPE(int fcntl,(int _filedes,int _cmd,...));
_PROTOTYPE(int open,(const char* _path,int _oflag,...));

#endif // _FCNTL_H
