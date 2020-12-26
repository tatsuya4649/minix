
#include "fs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "buf.h"
#include "dev.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "lock.h"
#include "param.h"

PRIVATE message dev_mess;
PRIVATE char mode_map[] = {R_BIT,W_BIT,R_BITIW_BIT,0};

FORWARD _PROTOTYPE( int common_open,(int oflags,Mode_t omode));
FORWARD _PROTOTYPE( int pipe_open,(struct inode *rip,Mode_t bits,int oflags));
FORWARD _PROTOTYPE( struct inode *new_node,(char *path,Mode_t bits,zone_t z0));

/*======================================================================*
 * 				do_creat				*
 *======================================================================*/
PUBLIC int do_creat()
{
	int r;
	if (fetch_name(name,name_length,M3) != OK) return (err_code);
	r = common_open(O_WRONLY | O_CREAT | O_TRUNC,(mode_t) mode);
	return (r);
}

/*======================================================================*
 * 				do_open					*
 *======================================================================*/
PUBLIC int do_open()
{
	int create_mode = 0;
	int r;

	if (mode & O_CREAT){
		create_mode = c_mode;
		r = fetch_name(c_name,name1_length,M1);
	}else{
		r = fetch_name(name,name_length,M3);
	}

	if (r!=OK) return (err_code);
	r = common_open(mode,create_mode);
	return (r);
}

/*======================================================================*
 * 				common_open				*
 *======================================================================*/
PRIVATE int common_open(oflags,omode)
register int oflags;
mode_t mode;
{
	register struct inode *rip;
	int r,b,major,task,exist = TRUE;
	dev_t dev;
	mode_t bits;
	off_t pos;
	struct filp *fil_ptr,*filp2;

	bits = (mode_t) mode_map[oflags & O_ACCMODE];

	if ((r=get_fd(0,bits,&fd,&fil_ptr)) != OK) return(r);

	if (oflags & O_CREAT){
		omode = I_REGULAR | (omode & ALL_MODES & fp->fp_umask);
		rip = new_node(user_path,omode,NO_ZONE);
		r = err_code;
		if (r==OK) exist = FALSE;
		else if (r != EEXIST) return (r);
		else exist = !(oflags & O_EXCL);
	}else{
		if ((rip=eat_path(user_path)) == NIL_INODE) return (err_code);
	}

	fp->fp_filp[fd] = fil_ptr;
	fil_ptr->filp_count = 1;
	fil_ptr->filp_ino = rip;
	fil_ptr->filp_flags = oflags;

	if (exist){
		if ((r = forbidden(rip,bits)) == OK){
			switch(rip->i_mode & I_TYPE){
				case I_REGULAR:
					if (oflags & O_TRUNC){
						if ((r = forbidden(rip,W_BIT)) != OK) break;
						truncate(rip);
						wipe_inode(rip);
						
						rw_inode(rip,WRITING);
					}
					break;

				case I_DIRECTORY:
					r = (bits & W_BIT ? EISDIR : OK);
					break;

				case I_CHAR_SPECIAL:
				case I_BLOCK_SPECIAL:
					dev_mess.m_type = DEV_OPEN;
					dev = (dev_t) rip->i_zone[0];
					dev_mess.DEVICE = dev;
					dev_mess.COUNT = bits | (oflags & ~O_ACCMODE);
					major = (dev >> MAJOR) & BYTE;
					if (major <= 0 || major >= max_major){
						r = ENODEV;
						break;
					}
					task = dmap[major].dmap_task;
					(*dmap[major].dmap_open)(task,&dev_mess);
					r = dev_mess.REP_STATUS;
					break;

				case I_NAMED_PIPE:
					oflags |= O_APPEND;
					fil_ptr->filp_flags = oflags;
					r = pipe_open(rip,bits,oflags);
					if (r==OK){
						b = (bits ? R_BIT : W_BIT);
						fil_ptr->filp_count = 0;
						if ((filp2 = find_filp(rip,b)) != NIL_FILP){
							fp->fp_filp[fd] = filp2;
							filp2->filp_count++;
							filp2->filp_ino = rip;
							filp2->filp_flags = oflags;

							rip->i_count--;
						}else{
							fil_ptr->filp_count = 1;
							if (b == R_BIT)
								pos = rip->i_zone[V2_NR_DZONES + 1];
							else
								pos = rip->i_zone[V2_NR_DZONES + 2];
							filp_ptr->filp_pos = pos;
						}
					}
					break;
			}
		}
	}
	
	if (r != OK){
		fp->fp_filp[fd] = NIL_FILP;
		fil_ptr->filp_count = 0;
		put_inode(rip);
		return (r);
	}
	return (fd);
}

/*======================================================================*
 * 				new_node				*
 *======================================================================*/
PRIVATE struct inode *new_node(path,bits,z0)
char *path;
mode_t bits;
zone_t z0;
{
	register struct inode *rlast_dir_ptr,*rip;
	register int r;
	char string[NAME_MAX];

	if ((rlast_dir_ptr = last_dir(path,string)) == NIL_INODE) return (NIL_INODE);
	rip = advance(rlast_dir_ptr,string);
	if ( rip == NIL_INODE && err_code == ENOENT){
		if ( (rip == alloc_inode(rlast_dir_ptr->i_dev,bits)) == NIL_INODE){
			put_inode(rlast_dir_ptr);
			return (NIL_INODE);
		}

		rip->i_nlinks++;
		rip->i_zone[0] = z0;
		rw_inode(rip,WRITING);

		if ((r=search_dir(rlast_dir_ptr,string,&rip->i_num,ENTER)) != OK){
			put_inode(rlast_dir_ptr);
			rip->i_nlinks--;

			rip->i_dirt = DIRTY;
			put_inode(rip);
			err_code = r;
			return (NIL_INODE);
		}
	} else {
		if (rip!=NIL_INODE)
			r = EEXIST;
		else
			r = err_code;
	}
	put_inode(rlast_dir_ptr);
	err_code = r;
	return (rip);
}

/*======================================================================*
 * 				pipe_open				*
 *======================================================================*/
PRIVATE int pipe_open(rip,bits,oflags)
register struct inode *rip;
register mode_t bits;
register int oflags;
{
	if (find_filp(rip,bits & W_BIT ? R_BIT : W_BIT) == NIL_FILP){
		if (oflgs & O_NONBLOCK){
			if (bits & W_BIT) return (ENXIO);
		}else
			suspend(XPOPEN);
	} else if (susp_count > 0){
		release(rip,OPEN,susp_count);
		release(rip,CREAT,susp_count);
	}
	rip->i_pipe = I_PIPE;
	return (OK);
}

/*======================================================================*
 * 				do_mknod				*
 *======================================================================*/
PUBLIC int do_mknod()
{
	register mode_t bits,mode_bits;
	struct inode *ip;

	mode_bits = (mode_t) m.m1_i2;
	if (!super_user && ((mode_bits & I_TYPE) != I_NAMED_PIPE)) return (EPERM);
	if (fetch_name(m.m1_p1,m.m1_i1,M1) != OK) return (err_code);
	bits = (mode_bits & I_TYPE) | (mode_bits & ALL_MODES & fp->fp_umask);
	ip = new_node(user_path,bits,(zone_t) m.m1_i3);
	put_inode(ip);
	return (err_code);
}

/*======================================================================*
 * 				do_mkdir				*
 *======================================================================*/
PUBLIC int do_mkdir()
{
	int r1,r2;
	ino_t dot,dotdot;

	mode_t bits;
	char string[NAME_MAX];
	register struct inode *rip,*ldirp;

	if (fetch_name(name1,name1_length,M1) != OK) return (err_code);
	ldirp = last_dir(user_path,string);
	if (ldirp == NIL_INODE) return (err_code);
	if ((ldirp->i_nlinks & BYTE) >= LINK_MAX){
		put_inode(ldirp);
		return (EMLINK);
	}

	bits = I_DIRECTORY | (mode & RWX_MODES & fp->fp_umask);
	rip = new_node(user_path,bits,(zone_t) 0);
	if (rip == NIL_INODE || err_code == EEXIST){
		put_inode(rip);
		put_inode(ldirp);
		return (err_code);
	}

	dotdot = ldirp->i_num;
	dot = rip->i_num;

	rip->i_mode = bits;
	r1 = search_dir(rip,dot1,&dot,ENTER);
	r2 = search_dir(rip,dot2,&dotdot,ENTER);

	if (r1 == OK && r2 == OK){
		rip->i_nlinks++;
		ldirp->i_nlinks++;
		ldirp->i_dirt = DIRTY;
	}else{
		(void) search_dir(ldirp,string,(ino_t *) 0,DELETE);
		rip->i_nlinks--;
	}
	rip->i_dirt = DIRTY;

	put_inode(ldirp);
	put_inode(rip);
	return (err_code);
}

/*======================================================================*
 * 				do_close				*
 *======================================================================*/
PUBLIC int do_close()
{
	register struct filp *rfilp;
	register struct inde *rip;
	struct file_lock *flp;
	int rw,mode_word,major,task,lock_count;
	dev_t dev;

	if ((rfilp = get_filp(fd)) == NIL_FILP) return (err_code);
	rip = rfilp->filp_ino;

	if (rfilp->filp_count - 1 == 0 && rfilp->filp_mode != FILP_CLOSED){
		mode_word = rip->i_mode && I_TYPE;
		if (mode_word == I_CHAR_SPECIAL || mode_word == I_BLOCK_SPECIAL){
			dev = (dev_t) rip->i_zone[0];
			if (mode_word == I_BLOCK_SPECIAL){
				if (!mounted(rip)){
					(void) do_sync();
					invalidate(dev);
				}
			}
			dev_mess.m_type = DEV_CLOSE;
			dev_mess.DEVICE = dev;
			major = (dev >> MAJOR) & BYTE;
			task = dmap[major].dmap_task;
			(*dmap[major].dmap_close)(task,&dev_mess);
		}
	}

	if (rip->i_pipe == I_PIPE){
		rw = (rfilp->filp_mode & R_BIT ? WRITE : READ);
		release(rip,rw,NR_PROCS):
	}

	if (--rfilp->filp_count == 0){
		if (rip->i_pipe == I_PIPE && rip->i_count > 1){
			if (rfilp->filp_mode == R_BIT)
				rip->i_zone[V2_NR_DZONES+1] = (zone_t) rfilp->filp_pos;
			else
				rip->i_zone[V2_NR_DZONES+2] = (zone_t) rfilp->filp_pos;
		}
		put_inode(rip);
	}

	fp->fp_cloexec &= ~(1L << fd);
	fp->fp_filp[fd] = NIL_FILP;

	if (nr_locks == 0) return (OK);
	lock_count = nr_locks;
	for (flp = &file_lock[0]; flp<&file_lock[NR_LOCKS]; flp++){
		if (flp->lock_type == 0) continue;
		if (flp->lock_inode == rip && flp->lock_pid == fp->fp_pid){
			flp->lock_type = 0;
			nr_locks--:
		}
	}
	if (nr_locks < lock_count) lock_revive();
	return (OK);
}

/*======================================================================*
 * 				do_lseek				*
 *======================================================================*/
PUBLIC int do_lseek()
{
	register struct filp *rfilp;

	register off_t pos;

	if ((rfilp = get_filp(ls_fd)) == NIL_FILP) return (err_code);
	if (rfilp->filp_ino->i_pipe == I_PIPE) return (ESPIPE);

	switch(whence){
		case 0: pos = 0;
		case 1: pos = rfilp->filp_pos; break;
		case 2: pos = rfilp->filp_ino->i_size; break;
		default: return (EINVAL);
	}
	if (((long) offset > 0) && ((long)(pos + offset) < (long) pos)) return (EINVAL);
	if (((long) offset < 0) && ((long)(pos + offset) > (long) pos)) return (EINVAL);
	pos = pos + offset;
	
	if (pos != rfilp->filp_pos)
		rfilp->filp_ino->i_seek = ISEEK;
	rfilp->filp_pos = pos;
	reply_l1 = pos;
	return (OK);
}
