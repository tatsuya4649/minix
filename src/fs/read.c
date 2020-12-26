#include "fs.h"
#include <fcntl.h>
#include <minix/com.h>
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "param.h"
#include "super.h"

#define FD_MASK		077

PRIVATE message umess;

FORWARD _PROTOTYPE( int rw_chunk,(struct inode *rip,off_t position,unsigned off,int chunk,unsigned left,
				int rw_flag,char *buff,int seg,int usr));

/*==============================================================*
 *				do_read				*
 *==============================================================*/
PUBLIC int do_read()
{
	return (read_write(READING));
}

/*==============================================================*
 *				read_write			*
 *==============================================================*/
PUBLIC int read_write(rw_flag)
int rw_flag;
{
	register struct inode *rip;
	register sturct filp *f;
	off_t bytes_left,f_size,position;
	unsigned int off,cum_io;
	int op,oflags,r,chunk,usr,seg,block_spec,char_spec;
	int regular,partial_pipe = 0,partial_cnt = 0;
	dev_t dev;
	mode_t mode_word;
	struct filp *wf;

	if (who == MM_PROC_NR && (fd & (~BYTE))){
		usr = (fd >> 8) & BYTE;
		seg = (fd >> 6) & 03;
		fd &= FD_MASK;
	}else{
		usr = who;
		seg = D;
	}
	if (nbytes < 0) return (EINVAL);
	if (( f = get_filp(fd)) == NIL_FILP) return (err_code);
	if (((f->filp_mode) & (rw_flag == READING ? R_BIT : W_BIT)) == 0){
		return (f->filp_mode == FILP_CLOSED ? EIO : EBADF);
	}
	if (nbytes == 0) return (0);

	position = f->filp_pos;
	if (position > MAX_FILE_POS) return (EINVAL);
	if (position + nbytes < position) return (EINVAL);
	oflags = f->filp_flags;
	rip = f->filp_ino;
	f_size = rip->i_size;
	r = OK;
	if (rip->i_pipe == I_PIPE){
		cum_io = fp->fp_cum_io_partial;
	}else{
		cum_io = 0;
	}
	op = (rw_flag == READING ? DEV_READ : DEV_WRITE);
	mode_word = rip->i_mode & I_TYPE;
	regular = mode_word == I_REGULAR || mode_word == I_NAMED_PIPE;

	char_spec = (mode_word == I_CHAR_SPECIAL ? 1 : 0);
	block_spec = (mode_word == I_BLOCK_SPECIAL ? 1 : 0);
	if (block_spec) f_size = LONG_MAX;
	rdwt_err = OK;

	if (char_spec){
		dev = (dev_t) rip->i_zone[0];
		r = dev_io(op,oflags & O_NONBLOCK,dev,position,nbytes,who,buffer);
		if (r >= 0){
			cum_io = r;
			position += r;
			r = OK;
		}
	}else{
		if (rw_flag == WRITING && block_spec == 0){
			if (position > rip->i_sp->s_max_size - nbytes) return (EFBIG);
			if (oflags & O_APPEND) position = f_size;
			if (position > f_size) clear_zone(rip,f_size,0);
		}
		if (rip->i_pipe == I_PIPE){
			r = pipe_check(rip,rw_flag,oflags,nbytes,position,&partial_cnt);
			if (r<=0) return (r);
		}
		if (partial_cnt > 0) partial_pipe = 1;

		while(nbytes != 0){
			off = (unsigned int) (position % BLOCK_SIZE);
			if (partial_pipe) {
				chunk = MIN(partial_cnt,BLOCK_SIZE - off);
			}else
				chunk = MIN(nbytes,BLOCK_SIZE - off);
			if (chunk < 0) chunk = BLOCK_SIZE - off;
			if (rw_flag == READING){
				bytes_left = f_size - position;
				if (position >= f_size) break;
				if (chunk > bytes_left) chunk = (int) bytes_left;
			}

			r = rw_chunk(rip,position,off,chunk,(unsigned) nbytes,rw_flag,buffer,seg,usr);
			if (r!=OK) break;
			if (rdwt_err < 0) break;
			
			buffer += chunk;
			nbytes -= chunk;
			cum_io += chunk;
			position += chunk;

			if (partial_pipe){
				partial_cnt -= chunk;
				if (partial_cnt <= 0) break;
			}
		}
	}

	if (rw_flag == WRITING){
		if (regular || mode_word == I_DIRECTORY){
			if (position > f_size) rip->i_size = position;
		}
	}else{
		if (rip->i_pipe == I_PIPE && position >= rip->i_size){
			rip->i_size = 0;
			position = 0;
			if ((wf = find_filp(rip,W_BIT)) != NIL_FILP) wf->filp_pos = 0;
		}
	}
	f->filp_pos = position;

	if (rw_flag == READING && rip->i_seek == NO_SEEK && position % BLOCK_SIZE == 0 && (regular || mode_word == I_DIRECTORY)){
		rdahed_inode = rip;
		rdahedpos = position;
	}
	rip->i_seek = NO_SEEK;

	if (rdwt_err != OK) r = rdwt_err;
	if (rdwt_err == END_OF_FILE) r = OK;
	if (r == OK){
		if (rw_flag == READING) rip->i_update |= ATIME;
		if (rw_flag == WRITING) rip->i_update |= CTIME | MTIME;
		rip->i_dirt = DIRTY;
		if (partial_pipe){
			partial_pipe = 0;
			if (!(oflags & O_NONBLOCK)){
				fp->fp_cum_io_partial = cum_io;
				suspend(XPIPE);
				return (0);
			}
		}
		fp->fp_cum_io_partial = 0;
		return (cum_io);
	}else{
		return (r);
	}
}

/*==============================================================*
 *				rw_chunk			*
 *==============================================================*/
 PRIVATE int rw_chunk(rip,position,off,chunk,left,rw_flag,buff,seg,usr)
 register struct inode *rip;
 off_t position;
 unsigned off;
 int chunk;
 unsigned left;
 int rw_flag;
 char *buff;
 int seg;
 int usr;
 {
	 register struct buf *bp;
	 register int r;
	 int n,blcok_spec;
	 block_t b;
	 dev_t dev;

	 block_spec = (rip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
	 if (block_spec){
		 b = position/BLOCK_SIZE;
		 dev = (dev_t) rip->i_zone[0];
	 }else{
		 b = read_map(rip,position);
		 dev = rip->i_dev;
	 }

	 if (!block_spec && b == NO_BLOCK){
		 if (rw_flag == READING){
			 bp = get_block(NO_DEV,NO_BLOCK,NORMAL);
			 zero_block(bp);
		 }else{
			 if ((bp=new_block(rip,position)) == NIL_BUF) return (err_code);
		 }
	 }else if (rw_flag == READING){
		 bp = rahead(rip,b,position,left);
	 }else{
		 n = (chunk == BLOCK_SIZE ? NO_READ : NORMAL);
		 if (!block_spec && off == 0 && position >= rip->i_size) n = NO_READ;
		 bp = get_block(dev,b,n);
	 }

	 if (rw_flag == WRITING && chunk != BLOCK_SIZE && !block_spec
		 && position >= rip->i_size && off == 0){
		 zero_block(bp);
	 }
	 if (rw_flag == READING){
		 r = sys_copy(FS_PROC_NR,D,(phys_bytes) (bp->b_data+off),
		 	usr,seg,(phys_bytes) buff,(phys_bytes) chunk);
	 }else{
		 r = sys_copy(usr,seg,(phys_bytes) buff,FS_PROC_NR,D,(phys_bytes) (bp->b_data+off),
		 	(phys_bytes) chunk);
		 bp->b_dirt = DIRTY;
	 }
	 n = (off + chunk == BLOCK_SIZE ? FULL_DATA_BLOCK : PARTIAL_DATA_BLOCK);
	 put_block(bp,n);
	 return (r);
 }

/*==============================================================*
 *				read_map			*
 *==============================================================*/
 PUBLIC block_t read_map(rip,position)
register struct inode *rip;
off_t position;
{
	register struct buf *bp;
	register zone_t z;
	int scale,boff,dzones,nr_indirects,index,zind,ex;
	block_t b;
	long excess,zone,block_pos;

	scale = rip->i_sp->s_log_zone_size;
	block_pos = position/BLOCK_SIZE;
	zone = block_pos >> scale;
	boff = (int) (block_pos - (zone << scale));
	dzones = rip->i_ndzones;
	nr_indirects = rip->i_nindirs;

	if (zone < dzones){
		zind = (int) zone;
		z = rip->i_zone[zind];
		if (z == NO_ZONE) return (NO_BLOCK);
		b = ((block_t) z << scale) + boff;
		return (b);
	}

	excess = zone - dzones;
	if (excess < nr_indirects){
		z = rip->i_zone[dzones];
	}else{
		if ((z = rip->i_zone[dzones+1])==NO_ZONE) return (NO_BLOCK);
		excess -= nr_indirects;
		b = (block_t) z << scale;
		bp = get_block(rip->i_dev,b,NORMAL);
		index = (int) (excess/nr_indirects);
		z = rd_indir(bp,index);
		put_block(bp,INDIRECT_BLOCK);
		excess = excess % nr_indirects;
	}

	if (z == NO_ZONE) return (NO_BLOCK);
	b = (block_t) z << scale;
	bp = get_block(rip->i_dev,b,NORMAL);
	ex = (int) excess;
	z = rd_indir(bp,ex);
	put_block(bp,INDIRECT_BLOCK);
	if (z == NO_ZONE) return (NO_BLOCK);
	b = ((block_t) z << scale) + boff;
	return (b);
}

/*==============================================================*
 *				rd_indir			*
 *==============================================================*/
PUBLIC zone_t rd_indir(bp,index)
struct buf *bp;
int index;
{
	struct super_block *sp;
	zone_t zone;

	sp = get_super(bp->b_dev);

	if (sp->s_version == V1)
		zone = (zone_t) conv2(sp->s_native, (int) bp->b_v1_ind[index]);
	else
		zone = (zone_t) conv4(sp->s_native, (long) bp->b_v2_ind[index]);
	
	if (zone != NO_ZONE &&
		(zone < (zone_t) sp->s_firstdatazone || zone >= sp->s_zones)){
		printf("Illegal zone nubmer %ld in indirect block, index %d\n",(long) zone,index);
		panic("check file system",NO_NUM);
	}
	return (zone);
}

/*==============================================================*
 *				read_ahead			*
 *==============================================================*/
 PUBLIC void read_ahead()
 {
	 register struct inode *rip;
	 struct buf *bp;
	 block_t b;

	 rip = rdahed_inode;
	 rdahed_inode = NIL_INODE;
	 if ((b=read_map(rip,rdahedpos)) == NO_BLOCK) return;
	 bp = rahead(rip,b,rdahedpos,BLOCK_SIZE);
	 put_block(bp,PARTIAL_DATA_BLOCK);
 }

/*==============================================================*
 *				rahead				*
 *==============================================================*/
 PUBLIC struct buf *rahead(rip,baseblock,position,bytes_ahead)
 register struct inode *rip;
 block_t baseblock;
 off_t position;
 unsigned bytes_ahead;
 {

#define BLOCKS_MINIMUM		(NR_BUFS < 50 ? 18 : 32)

	int block_spec,scale,read_q_size;
	unsigned int blocks_ahead,fragment;
	block_t block,blocks_left;
	off_t ind1_pos;
	dev_t dev;
	struct buf *bp;
	static struct buf *read_q[NR_BUFS];

	block_spec = (rip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
	if (block_spec){
		dev = (dev_t) rip->i_zone[0];
	}else{
		dev = rip->i_dev;
	}
	block = baseblock;
	bp = get_block(dev,block,PREFETCH);
	if (bp->b_dev != NO_DEV) return (bp);

	fragment = position % BLOCK_SIZE;
	position -= fragment;
	bytes_ahead += fragment;

	blocks_ahead = (bytes_ahead + BLOCK_SIZE - 1) / BLOCK_SIZE;

	if (block_spec && rip->i_size == 0){
		blocks_left = NR_IOREQS;
	}else{
		blocks_left = (rip->i_size - position + BLOCK_SIZE - 1)/BLOCK_SIZE;

		if (!block_spec){
			scale = rip->i_sp->s_log_zone_size;
			ind1_pos = (off_t) rip->i_ndzones * (BLOCK_SIZE << scale);
			if (position <= ind1_pos && rip->i_size > ind1_pos){
				blocks_ahead++;
				blocks_left++;
			}
		}
	}
	if (blocks_ahead > NR_IOREQS) blocks_ahead = NR_IOREQS;

	if (blocks_ahead < BLOCKS_MINIMUM && rip->i_seek == NO_SEEK)
		blocks_ahead = BLOCKS_MINIMUM;
	
	if (blocks_ahead > blocks_left) blocks_ahead = blocks_left;

	read_q_size = 0;

	for (;;){
		read_q[read_q_size++] = bp;

		if (--blocks_ahead == 0) break;

		if (bufs_in_use >= NR_BUFS - 4) break;

		block++;

		bp = get_block(dev,block,PREFETCH);
		if (bp->b_dev != NO_DEV){
			put_block(bp,FULL_DATA_BLOCK);
			break;
		}
	}
	rw_scattered(dev,read_q,read_q_size,READING);
	return (get_block(dev,baseblock,NORMAL));
}
