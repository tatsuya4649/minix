typedef struct{
	mode_t	d1_mode;
	uid_t	d1_uid;
	off_t	d1_size;
	time_t	d1_mtime;
	gid_t	d1_gid;
	nlink_t	d1_nlinks;
	u16_t	d1_zone[V1_NR_TZONES];

}d1_inode;

typedef struct{
	mode_t	d2_mode;
	u16_t	d2_nlinks;
	uid_t	d2_uid;
	u16_t	d2_gid;
	off_t	d2_size;
	time_t	d2_atime;
	time_t	d2_mtime;
	time_t	d2_ctime;
	zone_t	d2_zone[V2_NR_TZONES];
} d2_inode;
