
EXTERN struct file_lock{
	short lock_type;
	pid_t lock_pid;
	struct inode *lock_inode;
	off_t lock_first;
	off_t lock_last;
} file_lock[NR_LOCKS];
