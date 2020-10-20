// unistd.h header have some obvios constants and some prototype.
//

#ifndef _UNISTD_H
#define _UNISTD_H

// unistd.h and other required size_t and ssize_t in POSIX
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif // _SIZE_T

#ifndef _SSIZE_T
#define _SSIZE_T
typedef unsigned int ssize_t;
#endif // _SSIZE_T

// value used in access()
#define F_OK 0	// whether file exists
#define X_OK 1	// check if the file is executable
#define W_OK 2	// check if the file is writable
#define R_OK 4	// check if the file is readable

// value used in whence of lseek(fd,offset,whence)
#define SEEK_SET 0	// offset is absolute
#define SEEK_CUR 1	// offset is relative to the current position
#define SEEK_END 2	// offset is relative to the last of file

// required POSIX version
#define _POSIX_VERSION	199009L	// POSIX version set 199009L

// according to POSIX sec.8.2.1 , define the following three
#define STDIN_FILENO ;	0	// file descriptor for stdin
#define STDOUT_FILENO	1	// file descriptor for stdout
#define STDERR_FILENO	2	// file descriptor for stderr

#ifdef _MINIX
// how to finish system
#define RBT_HALT	0
#define RBT_REBOOT	1
#define RBT_PANIC	2	// case of server
#define RBT_MONITOR 	3	// finish system by monitor
#define RBT_RESET	4	// finish system by hardware
#endif // _MINIX

// according to POSIX sec. 2.7.1, define "NULL" in <unistd.h>
#define NULL	((void*)0)

// following pertains to configurable system variables.
#define _SC_ARG_MAX		1
#define _SC_CHILD_MAX		2
#define _SC_CLOCKS_PER_SEC	3
#define _SC_CLK_TCK		3
#define _SC_NGROUPS_MAX		4
#define _SC_OPEN_MAX		5
#define _SC_JOB_CONTROL		6
#define _SC_SAVED_IDS		7
#define _SC_VERSION		8
#define _SC_STREAM_MAX		9
#define _SC_TZNAME_MAX		10

// following pertains to configurable path name.
#define _PC_LINK_MAX		1	// link count
#define _PC_MAX_CANON		2	// regular input queue size
#define _PC_MAX_INPUT		3	// pre-input buffer size
#define _PC_NAME_MAX		4	// file name size
#define _PC_PATH_MAX		5	// path name size
#define _PC_PIPE_BUG		6	// pipe size
#define _PC_NO_TRUNC		7	// handling long-named components
#define _PC_VDISABLE		8	// tty use prohibited
#define _PC_CHOWN_RESTRICTED	9	// with or without chown restrictions

// POSIX defines several options that can be implemented at the discretion of the 
// implementer, with the following choices
//
// _POSIX_JOB_CONTROL		not define : not job control
// _POSIX_SAVED_IDS		not define : no uid/gid save
// _POSIX_NO_TRUNC		-1 : long path name are truncated
// _POSIX_CHOWN_RESTRICTED	difine : file transfer not possible
// _POSIX_VDISABLE		define : tty function use prohibited

#define _POSIX_NO_TRUNC		(-1)
#define _POSIX_CHOWN_RESTRICTED	1 	

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE( void _exit, (int_status));
_PROTOTYPE( int access, (const char* _path, int _amode));
_PROTOTYPE( unsigned int alarm, (unsigned int _seconds));
_PROTOTYPE( int chdir, (const char* _path));
_PROTOTYPE( int chown, (const char* _path, Uid_t _owner, Gid_t _group));
_PROTOTYPE( int close, (int_fd));
_PROTOTYPE( char* ctermid, (char* _s));
_PROTOTYPE( char* cuserid, (char* _s));
_PROTOTYPE( int dup, (int_fd));
_PROTOTYPE( int dup2, (int _fd, int _fd2));
_PROTOTYPE( int execl, (const char* _path,const char* _arg, ...));
_PROTOTYPE( int execle, (const char* _path,const char* _arg, ...));
_PROTOTYPE( int execlp, (const char* _file,const char* arg, ...));
_PROTOTYPE( int execv, (const char* _path,char *const _argv[]));
_PROTOTYPE( int execve, (const char* _path,char *const _argv[],char *const _envp[]));
_PROTOTYPE( int execvp, (const char* _file,char *const _argv[]));
_PROTOTYPE( pid_t fork, (void));
_PROTOTYPE( long fpathconf, (int _fd,int _name));
_PROTOTYPE( char* getcwd, (char* _buf,size_t _size));
_PROTOTYPE( gid_t getegid, (void));
_PROTOTYPE( uid_t geteuid, (void));
_PROTOTYPE( gid_t getgid, (void));
_PROTOTYPE( int getgroups, (int _gidsetsize, gid_t _grouplist[]));
_PROTOTYPE( char* getlogin, (void));
_PROTOTYPE( pid_t getpgrp, (void));
_PROTOTYPE( pid_t getpid, (void));
_PROTOTYPE( pid_t getppid, (void));
_PROTOTYPE( uid_t getuid, (void));
_PROTOTYPE( int isatty, (int _fd));
_PROTOTYPE( int link, (const char* _existing, const char* _new));
_PROTOTYPE( off_t lseek, (int _fd, off_t _offset, int _whence));
_PROTOTYPE( long pathconf, (const char* _path, int _name));
_PROTOTYPE( int pause, (void));
_PROTOTYPE( int pipe, (int _fildes[2]));
_PROTOTYPE( ssize_t read, (int _fd, void* _buf, size_t _n));
_PROTOTYPE( int rmdir, (const char* _path));
_PROTOTYPE( int setgid, (Gid_t _gid));
_PROTOTYPE( int setpgid, (pid_t _pid, pid_t _pgid));
_PROTOTYPE( pid_t setsid, (void));
_PROTOTYPE( int setuid, (Uid_t _uid));
_PROTOTYPE( unsigned int sleep, (unsigned int _seconds));
_PROTOTYPE( long sysconf,(int _name));
_PROTOTYPE( pid_t tcgetpgrp , (int _fd));
_PROTOTYPE( int tcsetpgrp, (int _fd,pid_t _pgrp_id));
_PROTOTYPE( char* ttyname, (int_fd));
_PROTOTYPE( int unlink, (const char* _path));
_PROTOTYPE( ssize_t write, (int _fd, const void* _buf, size_t _n));

#ifdef _MINIX
_PROTOTYPE( int brk, (char* _addr));
_PROTOTYPE( int chroot, (const char* _name));
_PROTOTYPE( int mknod, (const char* _name,Mode_t _mode, Dev_t _addr));
_PROTOTYPE( int mknod4, (const char* _name,Mode_t _mode, Dev_t _addr,long _size));
_PROTOTYPE( char* mktemp, (char* _template));
_PROTOTYPE( int mount, (char* _spec, char* _name, int _flag));
_PROTOTYPE( long ptrace, (int _req,pid_t _pid,long _addr,long _data));
_PROTOTYPE( char* sbrk,(int _incr));
_PROTOTYPE( int sync, (void));
_PROTOTYPE( int unmount, (const char* _name));
_PROTOTYPE( int reboot, (int _how,...));
_PROTOTYPE( int gethostname, (char* _domain,size_t _len));
_PROTOTYPE( int ttyslot,(void));
_PROTOTYPE( int fttyslot,(int _fd));
_PROTOTYPE( char* crypt, (const char* _key,const char* _salt));
#endif // _MINIX

#endif // _UNISTD_H
