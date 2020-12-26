
#include "fs.h"
#include <minix/com.h>
#include <minix/boot.h>
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "super.h"

FORWARD _PROTOTYPE( void rm_lru,(struct buf *bp));

/*======================================================*
 * 			get_block			*
 *======================================================*/
PUBLIC struct buf *get_block(dev,block,only_search)
register dev_t dev;
register block_t block;
int only_search;
{
	int b;
	register struct buf *bp,*prev_ptr;

	if (dev!=NO_DEV){
		b = (int) block & HASH_MASK;
		bp = buf_hash[b];
		while (bp != NIL_BUF){
			if (bp->b_blocknr == block && bp->b_dev == dev){
				if (bp->b_count == 0) rm_lru(bp);
				bp->b_count++;
				return (bp);
			}else{
				bp = bp->b_hash;
			}
		}
	}

	if ((bp=front) == NIL_BUF) panic("all buffers in use",NR_BUFS);
	rm_lru(bp);

	b = (int) bp->b_blocknr & HASH_MASK;
	prev_ptr = buf_hash[b];
	if (prev_ptr == bp){
		buf_hash[b] == bp->b_hash;
	}else{
		while(prev_ptr->b_hash != NIL_BUF)
			if (prev_ptr->b_hash == bp){
				prev_ptr->b_hash = bp->b_hash;
				break;
			}else{
				prev_ptr = prev_ptr->b_hash;
			}
	}

	if (bp->b_dev != NO_DEV){
		if (bp->b_dirt == DIRTY) flushall(bp->b_dev);
	}

	bp->b_dev = dev;
	bp->b_blocknr = block;
	bp->b_count++;
	b = (int) bp->b_blocknr & HASH_MASK;
	bp->b_hash = buf_hash[b];
	buf_hash[b] = bp;

	if (dev != NO_DEV){
		if (only_search == PREFETCH) bp->b_dev = NO_DEV;
		else
		if (only_search == NORMAL) rw_block(bp,READING);
	}
	return (bp);
}

/*======================================================*
 * 			put_block			*
 *======================================================*/
PUBLIC void put_block(bp,block_type)
register struct buf *bp;
int block_type;
{
	if (bp==NIL_BUF) return;
	
	bp->b_count--;
	if (bp->b_count != 0) return;

	bufs_in_use--;

	if (block_type & ONE_SHOT){
		bp->b_prev = NIL_BUF;
		bp->b_next = front;
		if (front == NIL_BUF)
			rear = bp;
		else
			front->b_prev = bp;
		front = bp;
	}else{
		bp->b_prev = rear;
		bp->b_next = NIL_BUF;
		if (rear == NIL_BUF)
			front = bp;
		else
			rear->b_next = bp;
		rear = bp;
	}

	if ((block_type & WRITE_IMMED) && bp->b_dirt == DIRTY && bp->b_dev != NO_DEV)
		rw_block(bp,WRITING);
}

/*======================================================*
 * 			alloc_zone			*
 *======================================================*/
PUBLIC zone_t alloc_zone(dev,z)
dev_t	dev;
zone_t  z;
{
	int major,minor;
	bit_t b,bit;
	struct super_block *sp;

	sp = get_super(dev);

	if (z == sp->s_firstdatazone){
		bit = sp->s_zsearch;
	}else{
		bit = (bit_t) z - (sp->s_firstdatazone - 1);
	}
	b = alloc_bit(sp,ZMAP,bit);
	if (b == NO_BIT){
		err_code = ENOSPC;
		major = (int) (sp->s_dev >> MAJOR) & BYTE;
		minor = (int) (sp->s_dev >> MINOR) & BYTE;
		printf("No space on %sdevice %d/%d\n",
				sp->s_dev == ROOT_DEV ? "root" : "",major,minor);
		return (NO_ZONE);
	}
	if (z == sp->s_fiestdatazone) sp->s_zsearch = b;
	return (sp->s_firstdatazone - 1 + (zone_t) b);
}

/*======================================================*
 * 			free_zone			*
 *======================================================*/
PUBLIC void free_zone(dev,numb)
dev_t dev;
zone_t numb;
{
	register struct super_block *sp;
	bit_t bit;

	sp = get_super(dev);
	if (numb < sp->s_firstdatazone || numb >= sp->s_zones) return;
	bit = (bit_t) (numb-(sp->s_firstdatazone-1));
	free_bit(sp,ZMAP,bit);
	if (bit<sp->s_zsearch) sp->s_zsearch = bit;
}

/*======================================================*
 * 			rw_block			*
 *======================================================*/
PUBLIC void rw_block(bp,rw_flag)
register struct buf *bp;
int rw_flag;
{
	int r,op;
	off_t pos;
	dev_t dev;

	if ((dev=bp->b_dev) != NO_DEV){
		pos = (off_t) bp->b_blocknr * BLOCK_SIZE;
		op = (rw_flag == READING ? DEV_READ : DEV_WRITE);
		r = dev_io(op,FALSE,dev,pos,BLOCK_SIZE,FS_PROC_NR,bp->b_data);
		if (r != BLOCK_SIZE){
			if (r>=0) r = END_OF_FILE;
			if (r != END_OF_FILE)
				printf("Unrecoverable disk error on device %d/%d,block %ld\n",
						(dev>>MAJOR) &BYTE,(dev>>MINOR) &BYTE,bp->b_blocknr);
				bp->b_dev = NO_DEV;
				if (rw_flag == READING) rdwt_err = r;
		}
	}
	bp->b_dirt = CLEAN;
}

/*======================================================*
 * 			invalidate			*
 *======================================================*/
PUBLIC void invalidate(device)
dev_t device;
{
	register struct buf *bp;
	for (bp=&buf[0];bp<&buf[NR_BUFS];bp++)
		if (bp->b_dev == device) bp->b_dev = NO_DEV;
}

/*======================================================*
 * 			flushall			*
 *======================================================*/
PUBLIC void flushall(dev)
dev_t dev;
{
	register struct buf *bp;
	static struct buf *dirty[NR_BUFS];
	int ndirty;

	for (bp=&buf[0],ndirty=0;bp<&buf[NR_BUFS];bp++)
		if(bp->b_dirt == DIRTY && bp->b_dev == dev) dirty[ndirty++] = bp;
	rw_scatterd(dev,dirty,ndirty,WRITING);
}

/*======================================================*
 * 			rw_scattered			*
 *======================================================*/
PUBLIC void rw_scattered(dev,bufq,bufqsize,rw_flag)
dev_t dev;
struct buf **bufq;
int bufqsize;
int rw_flag;
{
	register struct buf *bp;
	int gap;
	register int i;
	register struct iorequest_s *iop;
	static struct iorequest_t iovec[NR_IOREQS];
	int j;

	gap = 1;
	do
		gap = 3 * gap + 1;
	while(gap <= bufqsize);
	while(gap!=1){
		gap /= 3;
		for (j=gap;j<bufqsize;j++){
			for(i=j-gap;
					i>=0 && bufq[i]->b_blocknr > bufq[i+gap]->b_blocknr;
					i -= gap){
				bp = bufq[i];
				bufq[i] = bufq[i+gap];
				bufq[i+gap] = bp;
			}
		}
	}

	while(bufqsize > 0){
		for (j=0,iop=iovec;j<NR_IOREQ && j<bufqsize;j++,iop++){
			bp = bufq[j];
			iop->io_position = (off_t) bp->b_blocknr * BLOCK_SIZE;
			iop->io_buf = bp->b_data;
			iop->io_nbytes = BLOCK_SIZE;
			iop->io_request = rw_flag == WRITING ? DEV_WRITE : DEV_READ | OPTIONAL_IO;
		}
		(void) dev_io(SCATTERED_IO,0,dev,(off_t) 0,j,FS_PROC_NR,(char *) iovec);

		for (i=0,iop=iovec;i<j;i++,iop++){
			bp = bufq[i];
			if (rw_flag == READING){
				if (iop->io_nbytes == 0)
					bp->b_dev = dev;
				put_block(bp,PARTIAL_DATA_BLOCK);
			}else{
				if (iop->io_nbytes != 0){
					printf("Unrecoverable write error on device %d/%d,block %ld\m",
							(dev>>MAJOR)&BYTE,(dev>>MINOR)&BYTE,bp->b_blocknr);
					bp->b_dev = NO_DEV;
				}
				bp->b_dirt = CLEAN;
			}
		}
		bufq += j;
		bufqsize -= j;
	}
}


/*======================================================*
 * 			rm_lru				*
 *======================================================*/
PRIVATE void rm_lru(bp)
struct buf *bp;
{
	struct buf *next_ptr,*prev_ptr;
	bufs_in_use++;
	next_ptr = bp->b_next;
	prev_ptr = bp->b_prev;
	if (prev_ptr != NIL_BUF)
		prev_ptr->b_next = next_ptr;
	else
		front = next_ptr;

	if (next_ptr != NIL_BUF)
		next_ptr->b_prev = prev_ptr;
	else
		rear = prev_ptr;
}
