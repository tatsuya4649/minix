
#include "fs.h"
#include <string.h>
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "super.h"

FORWARD _PROTOTYPE( int write_map,(struct inode *rip,off_t position,zone_t new_zone));
FORWARD _PROTOTYPE( void wr_indir,(struct buf *bp,int index,zone_t zone));

/*======================================================*
 *			do_write			*
 *======================================================*/
PUBLIC int do_write()
{
	return (read_write(WRITING));
}

/*======================================================*
 *			write_map			*
 *======================================================*/
PRIVATE int write_map(rip,position,new_zone)
register struct inode *rip;
off_t position;
zone_t new_zone;
{
	int scale,ind_ex,new_ind,new_dbl,zones,nr_indirects,single,zindex,ex;
	zone_t z,z1;
	register block_t b;
	long excess,zone;
	struct buf *bp;

	rip->i_dirt = DIRTY;
	bp = NIL_BUF;
	scale = rip->i_sp->s_log_zone_size;
	zone = (position/BLOCK_SIZE) >> scale;
	zones = rip->i_ndzones;
	nr_indirects = rip->i_nindirs;

	if (zone < zones){
		zindex = (int) zone;
		rip->i_zone[zindex] = new_zone;
		return (OK);
	}

	excess = zone - zones;
	new_ind = FALSE;
	new_dbl = FALSE;

	if (excess < new_indirects){
		z1 = rip->i_zone[zones];
		single = TRUE;
	}else{
		if ((z = rip->i_zone[zones+1]) == NO_ZONE){
			if ((z = alloc_zone(rip->i_dev,rip->i_zone[0])) == NO_ZONE)
				return (err_code);
			rip->i_zone[zones+1] = z;
			new_dbl = TRUE;
		}
		excess -= nr_indirects;
		ind_ex = (int) (excess / nr_indirects);
		excess = excess % nr_indirects;
		if (ind_ex >= nr_indirects) return (EFBIG);
		b = (block_t) z << scale;
		bp = get_block(rip->i_dev,b,(new_dbl ? NO_READ : NORMAL));
		if (new_dbl) zero_block(bp);
		z1 = rd_indir(bp,ind_ex);
		single = FALSE;
	}

	if (z1 == NO_ZONE){
		z1 = alloc_zone(rip->i_dev,rip->i_zone[0]);
		if (single)
			rip->i_zone[zones] = z1;
		else
			wr_indir(bp,ind_ex,z1);

		new_ind = TRUE;
		if (bp != NIL_BUF) bp->b_dirt = DIRTY;

		if (z1 == NO_ZONE){
			put_block(bp,INDIRECT_BLOCK);
			return (err_code);
		}
	}
	put_block(bp,INDIRECT_BLOCK);

	b = (block_t) z1 << scale;
	bp = get_block(rip->i_dev,b,(new_ind ? NO_READ : NORMAL));
	if (new_ind) zero_block(bp);
	ex = (int) excess;
	wr_indir(bp,ex,new_zone);
	bp->b_dirt = DIRTY;
	put_block(bp,INDIRECT_BLOCK);

	return (OK);
}


/*======================================================*
 *			wr_indir			*
 *======================================================*/
