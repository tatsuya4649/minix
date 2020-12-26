

EXTERN struct super_block{
	ino_t		s_ninodes;
	zone1_t		s_nzones;
	short		s_imap_blocks;
	short		s_zmap_blocks;
	zone1_t		s_firstdatazone;
	short		s_log_zone_size;
	off_t		s_max_size;
	short		s_magic;
	short		s_pad;
	zone_t		s_zones;

	struct inode	*s_isup;
	struct inode	*s_imount;
	unsigned	s_inodes_per_block;
	dev_t		s_dev;
	int		s_rd_only;
	int		s_native;
	int		s_version;
	int		s_ndzones;
	int		s_nindirs;
	bit_t		s_isearch;
	bit_t		s_zsearch;
} super_block[NR_SUPERS];

#define NIL_SUPER	(struct super_block *)	0
#define IMAP		0
#define ZMAP		1

