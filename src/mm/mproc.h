

EXTERN struct mproc{
	struct mem_map	mp_seg[NR_SEGS];
	char		mp_exitstatus;
	char		mp_sigstatus;
	pid_t		mp_pid;
	pid_t		mp_procgrp;
	pid_t		mp_wpid;
	int		mp_parent;

	uid_t		mp_realuid;
	uid_t		mp_effuid;
	gid_t		mp_realgid;
	gid_t		mp_effgid;

	ino_t		mp_ino;
	dev_t		mp_dev;
	time_t		mp_ctime;

	sigset_t	mp_ignore;
	sigset_t	mp_catch;
	sigset_t	mp_sigmask;
	sigset_t	mp_sigmask2;
	sigset_t	mp_sigpending;
	struct sigaction mp_sigact[_NSIG+1];
	vir_bytes	mp_sigreturn;

	sighandler_t	mp_func;
	
	unsigned	mp_flags;
	vir_bytes	mp_procargs;
} mproc[NR_PROCS];


#define IN_USE		001
#define WAITING		002
#define HANGING		004
#define PAUSE		010
#define ALARM_ON	020
#define SEPARATE	040
#define TRACED		0100
#define STOPPED		0200
#define SIGSUSPENDED	0400

#define NIL_MPROC	((struct mproc *) 0)
