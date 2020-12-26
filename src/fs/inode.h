
EXTERN struct inode{
	mode_t i_mode;
	nlink_t i_nlinks;
	uid_t i_uid;
	gid_t i_gid;
	off_t i_size;
	time_t i_atime;
	time_t i_mtime;
	time_t i_ctime;
	zone_t i_zone[V2_NR_TZONES];

	dev_t i_dev;
	ino_t i_num;
	int i_count;

	int i_ndzones;
	int i_nindirs;
	struct super_block *i_sp;
	char i_dirt;
	char i_pipe;
	char i_mount;
	char i_seek;
	char i_update;
} inode[NR_INODES];

#define NIL_INODE	(struct inode *)  0

#define NO_PIPE		0
#define I_PIPE		1
#define NO_MOUNT	0
#define I_MOUNT		1
#define NO_SEEK		0
#define ISEEK		1
