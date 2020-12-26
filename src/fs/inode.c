

#include "fs.h"
#include <minix/boot.h>
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "super.h"

FORWARD _PROTOTYPE( void old_icopy, (struct inode *rip,d1_inode *dip,
			int direction,int norm));
FORWARD _PROTOTYPE( void new_icopy, (struct inode *rip,d2_inode *dip,
			int direction,int norm));

/*==============================================================*
 * 				get_inode			*
 *==============================================================*/
PUBLIC struct inode *get_inode(dev,numb)
dev_t dev;
int numb;
{
	register struct inode *rip,*xp;

	xp = NIL_INODE;
	for (rip=&inode[0];rip<&inode[NR_INODES];rip++){
		if (rip->i_count>0){
			if (rip->i_dev == dev && rip->i_num == numb){
				rip->i_count++;
				return(rip);
			}
		}else{
			xp = rip;
		}
	}
	if (xp == NIL_NODE){
		err_code = ENFILE;
		return (NIL_INODE);
	}

	xp->i_dev = dev;
	xp->i_num = numb;
	xp->i_count = 1;
	if (dev != NO_DEV) rw_inode(xp,READING);
	xp->i_update = 0;
	return (xp);
}

/*==============================================================*
 * 				put_inode			*
 *==============================================================*/
PUBLIC void put_inode(rip)
register struct inode *rip;
{
	if (rip==NIL_INODE) return;

	if (--rip->i_count == 0){
		if ((rip->i_nlinks & BYTE) == 0){
			truncate(rip);
			rip->i_mode = I_NOT_ALLOC;
			rip->i_dirt = DIRTY;
			free_inode(rip->i_dev,rip->i_num);
		}else{
			if (rip->i_pipe == I_PIPE) truncate(rip);
		}
		rip->i_pipe = NO_PIPE;
		if (rip->i_dirt == DIRTY) rw_inode(rip,WRITING);
	}
}

/*==============================================================*
 * 				alloc_inode			*
 *==============================================================*/
PUBLIC struct inode *alloc_inode(dev,bits)
dev_t dev;
mode_t bits;
{
	register struct inode *rip;
	register struct super_block *sp;
	int major,minor,inumb;
	bit_t b;

	sp = get_super(dev);
	if (sp->s_rd_only){
		err_code = EROFS;
		return (NIL_INODE);
	}

	b = alloc_bit(sp,IMAP,sp->s_isearch);
	if (b == NO_BIT){
		err_code = ENFILE;
		major = (int) (sp->s_dev >> MAJOR) & BYTE;
		minor = (int) (sp->s_dev >> MINOR) & BYTE;
		printf("Out of i-nodes on %sdevice %d/%d\n",
				sp->s_dev == ROOT_DEV ? "root " : "",major,minor);
		return (NIL_INODE);
	}
	sp->s_isearch = b;
	inumb = (int) b;

	if ((rip = get_inode(NO_DEV,inumb)) == NIL_INODE){
		free_bit(sp,IMAP,b);
	}else{
		rip->i_mode = bits;
		rip->i_nlinks = (nlink_t) 0;
		rip->i_uid = fp->fp_effuid;
		rip->i_gid = fp->fp_effgid;
		rip->i_dev = dev;
		rip->i_ndzones = sp->s_ndzones;
		rip->i_nindirs = sp->s_nindirs;
		rip->i_sp = sp;

		wipe_inode(rip);
	}

	return (rip);
}

/*==============================================================*
 * 			wipe_inode				*
 *==============================================================*/
PUBLIC void wipe_inode(rip)
register struct inode *rip;
{
	register int i;
	rip->i_size = 0;
	rip->i_update = ATIME | CTIME | MTIME;
	rip->i_dirt = DIRTY;
	for(i=0;i<V2_NR_TZONES;i++) rip->i_zone[i] = NO_ZONE;
}

/*==============================================================*
 * 			free_inode				*
 *==============================================================*/
PUBLIC void free_inode(dev,inumb)
dev_t dev;
ino_t inumb;
{
	register struct super_block *sp;
	bit_t b;

	sp = get_super(dev);
	if (inumb <= 0 || inumb > sp->s_ninodes) return;
	b = inumb;
	free_bit(sp,IMAP,b);
	if (b < sp->s_isearch) sp->s_isearch = b;
}

/*==============================================================*
 * 			update_times				*
 *==============================================================*/
PUBLIC void update_times(rip)
register struct inode *rip;
{
	time_t cur_time;
	struct super_block *sp;

	sp = rip->i_sp;
	if (sp->s_rd_only) return;

	cur_time = clock_time();
	if (rip->i_update & ATIME) rip->i_atime = cur_time;
	if (rip->i_update & CTIME) rip->i_ctime = cur_time;
	if (rip->i_update & MTIME) rip->i_mtime = cur_time;
	rip->i_update = 0;
}

/*==============================================================*
 * 			rw_inode				*
 *==============================================================*/
PUBLIC void rw_inode(rip,rw_flag)
register struct inode *rip;
int rw_flag;
{

	register struct buf *bp;
	register struct super_block,*sp;
	d1_inode *dip;
	d2_inode *dip2;
	block_t b,offset;

	sp = get_super(rip->i_dev);
	rip->i_sp = sp;
	offset = sp->s_imap_blocks + sp->s_zmap_blocks + 2;
	b = (block_t) (rip->i_num - 1)/sp->s_inodes_per_block + offset;
	bp = get_block(rip->i_dev,b,NORMAL);
	dip = bp->b_v1_ino + (rip->i_num - 1) % V1_INODES_PER_BLOCK;
	dip2 = bp->b_v2_ino + (rip->i_num - 1) % V2_INODES_PER_BLOCK;

	if (rw_flag == WRITING){
		if (rip->i_update) update_times(rip);
		if (sp->s_rd_only == FALSE) bp->b_dirt = DIRTY;
	}

	if (sp->s_version == V1)
		old_icopy(rip,dip,rw_flag,sp->s_native);
	else
		new_icopy(rip,dip,rw_flag,sp->s_native);
	put_block(bp,INODE_BLOCK);
	rip->i_dirt = CLEAN;
}

/*==============================================================*
 * 			old_icopy				*
 *==============================================================*/
PRIVATE void old_icopy(rip,dip,direction,norm)
register struct inode *rip;
register d1_inode *dip;
int direction;
int norm;
{
	int i;

	if (direction == READING){
		rip->i_mode = conv2(norm,(int) dip->d1_mode);
		rip->i_uid = conv2(norm,(int) dip->d1_uid);
		rip->i_size = conv4(norm, dip->d1_size);
		rip->i_mtime = conv4(norm,dip->d1_mtime);
		rip->i_atime = rip->i_mtime;
		rip->i_ctime = rip->i_mtime;
		rip->i_nlinks = (nlink_t) dip->d1_nlinks;
		rip->i_gid = (gid_t) dip->d1_gid;
		rip->i_ndzones = V1_NR_DZONES;
		rip->i_nindirs = V1_INDIRECTS;
		for (i=0;i<V1_NR_TZONES;i++)
			rip->i_zone[i] = conv2(norm,(int) dip->d1_zone[i]);
	} else{
		dip->d1_mode = conv2(norm,(int) rip->i_mode);
		dip->d1_uid = conv2(norm, (int) rip->i_uid);
		dip->d1_size = conv4(norm, rip->i_size);
		dip->d1_mtime = conv4(norm,rip->i_size);
		dip->d1_nlinks = (nlink_t) rip->i_nlinks;
		dip->d1_gid = (gid_t) rip->i_gid;
		for (i=0;i<V1_NR_TZONES;i++)
			dip->d1_zone[i] = conv2(norm,(int) rip->i_zone[i]);
	}
}

/*==============================================================*
 * 			new_icopy				*
 *==============================================================*/
PRIVATE void new_icopy(rip,dip,direction,norm)
register struct inode *rip;
register d2_inode *dip;
int direction;
int norm;
{
	int i;
	if (direction == READING){
		rip->i_mode = conv2(norm,dip->d2_mode);
		rip->i_uid = conv2(norm,dip->d2_uid);
		rip->i_nlinks = conv2(norm,(int) dip->d2_nlinks);
		rip->i_gid = conv2(norm,(int) dip->d2_gid);
		rip->i_size = conv4(norm,dip->d2_size);
		rip->i_atime = conv4(norm,dip->d2_atime);
		rip->i_ctime = conv4(norm,dip->d2_ctime);
		rip->i_mtime = conv4(norm,dip->d2_mtime);
		rip->i_ndzones = V2_NR_DZONES;
		rip->i_nindirs = V2_INDIRECTS;
		for (i=0;i<V2_NR_TZONES;i++)
			rip->i_zone[i] = conv4(norm,(long) dip->d2_zone[i]);
	}else{
		dip->d2_mode = conv2(norm,rip->i_mode);
		dip->d2_uid = conv2(norm,rip->i_uid);
		dip->d2_nlinks = conv2(norm,rip->i_nlinks);
		dip->d2_gid = conv2(norm,rip->i_gid);
		dip->d2_size = conv4(norm,rip->i_size);
		dip->d2_atime = conv4(norm,rip->i_atime);
		dip->d2_ctime = conv4(norm,rip->i_ctime);
		dip->d2_mtime = conv4(norm,rip->i_mtime);
		for (i=0;i<V2_NR_TZONES;i++)
			dip->d2_zone[i] = conv4(norm,(long) rip->i_zone[i]);
	}
}

/*==============================================================*
 * 			dup_inode				*
 *==============================================================*/
PUBLIC void dup_inode(ip)
struct inode *ip;
{
	ip->i_count++;
}
