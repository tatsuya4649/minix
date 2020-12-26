
#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

EXTERN struct fproc *fp;
EXTERN int super_user;
EXTERN int dont_reply;
EXTERN int susp_count;
EXTERN int nr_locks;
EXTERN int reviving;
EXTERN off_t rdahedpos;
EXTERN struct inode *rdahed_inode;

EXTERN message m;
EXTERN message m1;
EXTERN int who;
EXTERN int fs_call;
EXTERN char user_path[PATH_MAX];

EXTERN int err_code;
EXTERN int rdwt_err;

extern _PROTOTYPE( int (*call_vector[]),(void));
extern int max_major;
extern char dot1[2];
extern char dot2[3];
