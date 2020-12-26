
#include <sys/dir.h>

EXTERN struct buf{
	union{
		char b__data[BLOCK];
		struct direct b__dir[NR_DIR_ENTRIES];
		zone1_t	b__v1_ind[V1_INDIRECTS];
		zone_t	b__v2_ind[V2_INDIRECTS];
		d1_inode b__v1_ino[V1_INODES_PER_BLOCK];
		d2_inode b__v2_ino[V2_INODES_PER_BLOCK];
		bitchunk_t b__bitmap[BITMAP_CHUNKS];
	} b;

	struct buf *b_next;
	struct buf *b_prev;
	struct buf *b_hash;
	block_t b_blocknr;
	dev_t b_dev;
	char b_dirt;
	char b_count;
}buf[NR_BUFS];

#define NIL_BUF		((struct buf *) 0)

#define b_data		b.b__data
#define b_dir		b.b__dir
#define b_v1_ind	b.b__v1_ind
#define b_v2_ind	b.b__v2_ind
#define b_v1_ino	b.b__v1_ino
#define b_v2_ino	b.b__v2_ino
#define b_bitmap	b.b__bitmap

EXTERN struct buf *buf_hash[NR_BUF_HASH];

EXTERN struct buf *front;
EXTERN struct buf *rear;
EXTERN int bufs_in_use;

#define WRITE_IMMED	0100
#define ONE_SHOT	0200

#define INODE_BLOCK		(0+MAYBE_WRITE_IMMED)
#define DIRECTORY_BLOCK		(1+MAYBE_WRITE_IMMED)
#define INDIRECT_BLOCK		(2+MAYBE_WRITE_IMMED)
#define MAP_BLOCK		(3+MAYBE_WRITE_IMMED)
#define ZUPER_BLOCK		(4+MAYBE_WRITE_IMMED)
#define FULL_DATA_BLOCK		5
#define PARTIAL_DATA_BLOCK	6

#define HASH_MASK		(NR_BUF_HASH - 1)
