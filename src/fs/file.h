

EXTERN struct filp{
	mode_t filp_mpde;
	int filp_flags;
	int filp_count;
	struct inode *filp_ino;
	off_t filp_pos;
} filp[NR_FILPS];

#define FILP_CLOSED	0
#define NIL_FILP	(struct file *) 0
