
#define V1_NR_DZONES	7
#define V1_NR_TZONES	9
#define V2_NR_DZONES	7
#define V2_NR_TZONES	10

#define NR_FILPS	128
#define NR_INODES	64
#define NR_SUPERS	8
#define NR_LOCKS	8

#define usizeof(t)	((unsigned) sizeof(t))

#define SUPER_MAGIC	0x137F
#define SUPER_REV	0x7F13

#define SUPER_V2	0x2468
#define SUPER_V2_REV	0x6824

#define	V1		1
#define V2		2

#define SU_UID		((uid_t) 0)
#define SYS_UID		((uid_t) 0)
#define SYS_GID		((uid_t) 0)
#define NORMAL		0
#define NO_READ		1
#define PREFETCH	2

#define	XPIPE		(-NR_TASKS-1)
#define XOPEN		(-NR_TASKS-2)
#define XLOCK		(-NR_TASKS-3)
#define XPOPEN		(-NR_TASKS-4)

#define NO_BIT		((bit_t) 0)

#define DUP_MASK	0100

#define LOCK_UP		0
#define ENTER		1
#define DELETE		2
#define IS_EMPTY	3

#define CLEAN		0
#define DIRTY		1
#define ATIME		002
#define CTIME		004
#define MTIME		010

#define BYTE_SWAP	0
#define DONT_SWAP	1

#define END_OF_FILE	(-104)

#define ROOT_INODE	1
#define BOOT_BLOCK	((block_t) 0)
#define SUPER_BLOCK	((block_t) 1)

#define DIR_ENTRY_SIZE	usizeof (struct direct)
#define NR_DIR_ENTRIES	(BLOCK_SIZE/DIR_ENTRY_SIZE)

#define SUPER_SIZE	usizeof (struct super_block)
#define PIPE_SIZE	(V1_NR_DZONES*BLOCK_SIZE)
#define BITMAP_CHUNKS	(BLOCK_SIZE/usizeof (bitchunk_t))

#define V1_ZONE_NUM_SIZE	usizeof (zone1_t)
#define V1_INODE_SIZE		usizeof (d1_inode)
#define V1_INDIRECTS		(BLOCK_SIZE/V1_ZONE_NUM_SIZE)
#define V1_INODES_PER_BLOCK	(BLOCK_SIZE/V1_INODE_SIZE)

#define V2_ZONE_NUM_SIZE	usizeof (zone_t)
#define V2_INODE_SIZE		usizeof (d2_inode)
#define V2_INDIRECTS		(BLOCK_SIZE/V2_ZONE_NUM_SIZE)
#define V2_INODES_PER_BLOCK	(BLOCK_SIZE/V2_INODE_SIZE)

#define printf	printk

