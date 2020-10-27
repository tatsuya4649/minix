// <minix/callnr.h> header that includes number of system calls.
//
//

#define NCALLS		77		// number of allowed system calls

#define	EXIT		1
#define FORK		2
#define READ		3
#define WRITE		4
#define OPEN		5
#define CLOSE		6
#define WAIT		7
#define CREAT		8
#define LINK		9
#define UNLINK		10
#define WAITPID		11
#define CHDIR		12
#define TIME		13
#define MKNOD		14
#define CHMOD		15
#define CHOWN		16
#define BRK		17
#define STAT		18
#define LSEEK		19
#define GETPID		20
#define MOUNT		21
#define UMOUND		22
#define SETUID		23
#define GETUID		24
#define	STIME		25
#define PTRACE		26
#define ALARM		27
#define FSTAT		28
#define PAUSE		29
#define UTIME		30
#define ACCESS		33
#define SYNC		36
#define KILL		37
#define RENAME		38
#define MKDIR		39
#define RMDIR		40
#define DUP		41
#define PIPE		42
#define TIMES		43
#define SETGID		46
#define GETGID		47
#define SIGNAL		48
#define IOCTL		54
#define FCNTL		55
#define EXEC		59
#define UMASK		60
#define CHROOT		61
#define SETSID		62
#define GETPGRP		63

// the following is not a system call , but it is processed in the same way as a system call.
#define KSIG		64	// the kernel has detected a signal
#define UNPAUSE		65	// for MM or FS : check EINTR
#define REVIVE		67	// for FS : recover from the dormant process
#define TASK_REPLY	68	// for FS : return code from tty task

// Posix signal handle
#define SIGACTION	71
#define SIGSUSPEND	72
#define SIGPENDING	73
#define SIGPROCMASK	74
#define SIGRETURN	75

#define REBOOT		76
