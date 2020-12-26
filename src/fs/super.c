
#include "fs.h"
#include <string.h>
#include <minix/boot.h>
#include "buf.h"
#include "inode.h"
#include "super.h"

#define BITCHUNK_BITS	(usizeof(bitchunk_t) * CHAR_BIT)
#define BITS_PER_BLOCK	(BITMAP_CHUNKS * BITCHUNK_BITS)

/*======================================================*
 * 			alloc_bit			*
 *======================================================*/
PUBLIC bit_t alloc_bit(sp,map,origin)
struct super_block *sp;
int map;
bit_t origin;
{
	block_t start_block;
	bit_t	map_bits;
	unsigned bit_blocks;
	unsigned block,word,bcount,wcount;
	struct buf *bp;
	bitchunk_t *wptr,*wlim,k;
	bit_t i,b;

	if (sp->s_rd_only)
		panic("can't allocate bit on read-only filesys.",NO_NUM);
	if (map==IMAP){
		start_block = SUPER_BLOCK + 1;
		map_bits = sp->s_ninodes + 1;
		bit_blocks = sp->s_imap_blocks;
	}else{
		start_block = SUPER_BLOCK + 1 + sp->s_imap_blocks;
		map_bits = sp->s_zones - (sp->s_firstdatazone - 1);
		bit_blocks = sp->s_zmap_blocks;
	}

	if (origin >= map_bits) origin = 0;

	block = origin / BITS_PER_BLOCK;
	word = (origin % BITS_PER_BLOCK) / BITCHUNL_BITS;

	bcount = bit_blocks + 1;
	do{
		bp = get_block(sp->s_dev,start_block + block,NORMAL);
		wlim = &bp->b_bitmap[BITMAP_CHUNKS];

		for (wptr=&bp->b_bitmap[word];wptr<wlim;wptr++){
			if (*wptr == (bitchunk_t) ~0) continue;
			
			k = conv2(sp->s_native,(int) *wptr);
			for (i=0;(k & (1<<i)) != 0; ++i) {}
			
			b = ((bit_t) block * BITS_PER_BLOCK)
				+ (wptr - &bp->b_bitmap[0]) * BITCHUNK_BITS + i;

			if (b >= map_bits) break;

			k |= 1 << i;
			*wptr = conv2(sp->s_native,(int) k);
			bp->b_dirt = DIRTY;
			put_block(bp,MAP_BLOCK);
			return (b);
		}
		put_block(bp,MAP_BLOCK);
		if (++block >= bit_blocks) block=0;
		word = 0;
	} while(--bcount>0);
	return (NO_BIT);
}

/*======================================================*
 * 			free_bit			*
 *======================================================*/
PUBLIC void free_bit(sp,map,bit_returned)
struct super_block *sp;
int map;
bit_t bit_returned;
{
	unsigned block,word,bit;
	struct buf *bp;
	bitchunk_t k,mask;
	block_t start_block;

	if (sp->s_rd_only)
		panic("can't free bit on read-only filesys.",NO_NUM);
	if (map==IMAP){
		start_block = SUPER_BLOCK + 1;
	}else{
		start_block = SUPER_BLOCK + 1 + sp->s_imap_blocks;
	}
	block = bit_returned / BITS_PER_BLOCK;
	word = (bit_returned % BITS_PER_BLOCK) / BITCHUNK_BITS;
	bit = bit_returned % BITCHUNK_BITS;
	mask = 1 << bit;

	bp = get_block(sp->s_dev,start_block + block,NORMAL);

	k = conv2(sp->s_native,(int) bp->b_bitmap[word]);
	if (!(k&mask)){
		panic(map==IMAP ? "tried to free unused inode" : "tried to free unused block",NO_NUM);
	}
	k &= ~mask;
	bp->b_bitmap[word] = conv2(sp->s_native,(int) k);
	bp->b_dirt = DIRTY;

	put_block(bp,MAP_BLOCK);
}
	
/*======================================================*
 * 			get_super			*
 *======================================================*/
PUBLIC struct super_block *get_super(dev)
dev_t dev;
{
	register struct super_block *sp;

	for (sp=&super_block[0];sp<&super_block[NR_SUPERS];sp++)
		if (sp->s_dev == dev) return (sp);

	panic("can't find superblock for device (in decimal)",(int) dev);
	return (NIL_SUPER);
}

/*======================================================*
 * 			mounted				*
 *======================================================*/
PUBLIC int mounted(rip)
register struct inode *rip;
{
	register struct super_block *sp;
	register dev_t dev;
	dev = (dev_t) rip->i_zone[0];
	if (dev == ROOT_DEV) return (TRUE);

	for (sp=&super_block[0];sp<&super_block[NR_SUPERS];sp++)
		if (sp->s_dev == dev) return (TRUE);
	
	return (FALSE);
}

/*======================================================*
 * 			read_super			*
 *======================================================*/
PUBLIC int read_super(sp)
register struct super_block *sp;
{
	register struct buf *bp;
	dev_t dev;
	int magic;
	int version,native;

	dev = sp->s_dev;
	bp = get_block(sp->s_dev,SUPER_BLOCK,NORMAL);
	memcpy((char *) sp,bp->b_data,(size_t) SUPER_SIZE);
	put_block(bp,ZUPER_BLOCK);
	sp->s_dev = NO_DEV;
	magic = sp->s_magic;

	if (magic == SUPER_MAGIC || magic == conv2(BYTE_SWAP,SUPER_MAGIC)){
		version = V1;
		native = (magic == SUPER_MAGIC);
	}else if (magic == SUPER_V2 || magic == conv2(BYTE_SWAP,SUPER_V2)){
		version = V2;
		native = (magic == SUPER_V2);
	}else{
		return (EINVAL);
	}

	sp->s_ninodes = conv2(native,(int) sp->s_ninodes);
	sp->s_nzones = conv2(native,(int) sp->s_nzones);
	sp->s_imap_blocks = conv2(native,(int) sp->s_imap_blocks);
	sp->s_zmap_blocks = conv2(native,(int) sp->s_zmap_blocks);
	sp->s_firstdatazone = conv2(native,(int) sp->s_firstdatazone);
	sp->s_log_zone_size = conv2(native,(int) sp->s_log_zone_size);
	sp->s_max_size = conv4(native,sp->s_max_size);
	sp->s_zones = conv4(native,sp->s_zones);

	if (version == V1){
		sp->s_zones = sp->nzones;
		sp->s_inodes_per_block = V1_INODES_PER_BLOCK;
		sp->s_ndzones = V1_NR_DZONES;
		sp->s_nindirs = V1_INDIRECTS;
	}else{
		sp->s_inodes_per_block = V2_INODES_PER_BLOCK;
		sp->s_ndzones = V2_NR_DZONES;
		sp->s_nindirs = V2_INDIRECTS;
	}

	sp->s_isearch = 0;
	sp->s_zsearch = 0;
	sp->s_version = version;
	sp->s_native = native;

	if (sp->s_imap_blocks < 1 || sp->s_zmap_blocks < 1 ||
			sp->s_ninodes < 1 || sp->s_zones < 1 || (unsigned) sp->s_log_zone_size > 4){
		return (EINVAL);
	}
	sp->s_dev = dev;
	return (OK);
}
