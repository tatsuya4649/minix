



EXTERN struct fproc{
	mode_t fp_umask;
	struct inode *fp_workdir;
	struct inode *fp_rootdir;
	struct filp *fp_flip[OPEN_MAX];
	uid_t fp_realuid;
	uid_t fp_effuid;
	gid_t fp_realgid;
	gid_t fp_effgid;
	dev_t fp_tty;
	int fp_fd;
	char *fp_buffer;
	int fp_nbytes;
	int fp_cum_io_partial;
	char fp_suspended;
	char fp_revived;
	char fp_task;
	char fp_sesldr;
	pid_t fp_pid;
	long fp_cloexec;
} fproc[NR_PROCS];

#define NOT_SUSPENDED	0
#define SUSPENDED	1
#define NOT_REVIVING	0
#define REVIVING	1
