
#include "fs.h"
#include <fcntl.h>
#include <signal.h>
#include <minix/boot.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "dev.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "param.h"

PRIVATE message mess;

/*==============================================================*
 *				do_pipe 			*
 *==============================================================*/
PUBLIc int do_pipe()
{
	register struct fproc *rfp;
	register struct inode *fip;
	int r;
	struct filp *fil_ptr0,*fil_ptr1;
	int fil_des[2];

	rfp = fp;
	if ((r = get_fd(0,R_BIT,&fil_des[0],&fil_ptr0))!=OK){
		rfp->fp_filp[fil_des[0]] = NIL_FILP;
		fil_ptr0->filp_count = 0;
		return (r);
	}
	rfp->fp_filp[fil_des[1]] = fil_ptr1;
	fil_ptr1->filp_count = 1;

	if ((rip=alloc_inode(PIPE_DEV,I_REGULAR)) == NIL_INODE){
		rfp->fp_filp[fil_des[0]] = NIL_FILP;
		fil_ptr0->filp_count = 0;
		rfp->fp_filp[fil_des[1]] = NIL_FILP;
		fil_ptr1->filp_count = 0;
		return (err_code);
	}

	if (read_only(rip) != OK) panic("pipe device is read only",NO_NUM);

	rip->i_pipe = I_PIPE;
	rip->i_mode &= I_REGULAR;
	rip->i_mode |= I_NAMED_PIPE;
	fil_ptr0->filp_ino = rip;
	fil_ptr0->filp_flags = O_RDONLY;
	dup_inode(rip);
	fil_ptr1->filp_ino = rip;
	fil_ptr1->filp_flags = O_WRONLY;
	rw_inode(rip,WRITING);
	reply_i1 = fil_des[0];
	reply_i2 = fil_des[1];
	rip->i_update = ATIME | CTIME | MTIME;
	return (OK);
}

/*==============================================================*
 *				pipe_check 			*
 *==============================================================*/
PUBLIC int pipe_check(rip,rw_flag,oflags,bytes,position,canwrite)
register struct inode *rip;
int rw_flag;
int oflags;
register int bytes;
register off_t position;
int *canwrite;
{
	int r = 0;
	
	if (rw_flag == READING){
		if (position >= rip->i_size){
			if (find_filp(rip,W_BIT) != NIL_FILP){
				if (oflags & O_NONBLOCK)
					r = EAGAIN;
				else
					suspend(XPIPE);
				if (susp_count > 0) release(rip,WRITE,susp_count);
			}
			return (r);
		}
	}else{
		if (find_filp(rip,R_BIT) == NIL_FILP){
			sys_kill((int)(fp - fproc),SIGPIPE);
			return (EPIPE);
		}

		if (position + bytes > PIPE_SIZE){
			if ((oflags & O_NONBLOCK) && bytes < PIPE_SIZE)
				return (EAGAIN);
			else if ((oflags & O_NONBLOCK) && bytes > PIPE_SIZE){
				if ((*canwrite = (PIPE_SIZE - position) > 0){
					release(rip,READ,susp_count);
					return (1);
				}else{
					return (EAGAIN);
				}
			}
			if (bytes > PIPE_SIZE){
				if ((*canwrite = PIPE_SIZE - position) > 0){
					release(rip,READ,susp_count);
					return (1);
				}
			}
			suspend(XPIPE);
			return (0);
		}
		if (position == 0) release(rip,READ,susp_count);
	}
	*canwrite = 0;
	return (1);
}

/*==============================================================*
 *				suspend 			*
 *==============================================================*/
PUBLIC void suspend(task)
int task;
{
	if (task == XPIPE || task == XPOPEN) susp_count++;
	fp->fp_suspended = SUSPEND;
	fp->fp_fd = fd << 8 | fs_call;
	fp->fp_task = -task;
	if (task == XLOCK){
		fp->fp_buffer = (char *) name1;
		fp->fp_nbytes = request;
	}else{
		fp->fp_buffer = buffer;
		fp->fp_nbytes = nbytes;
	}
	dont_reply = TRUE;
}

/*==============================================================*
 *				release 			*
 *==============================================================*/
PUBLIC void release(ip,call_nr,count)
register struct inode *ip;
int call_nr;
int count;
{
	register struct fproc *rp;

	for (rp=&fproc[0];rp < &fproc[NR_PROC];rp++){
		if (rp->fp_suspended == SUSPENDED &&
				rp->fp_revived == NOT_REVIVING &&
				(rp->fp_fd & BYTE) == call_nr &&
				rp->fp_filp[rp->fp_fd>>8]->filp_ino==ip){
			revive((int)(rp - fproc),0);
			susp_count--;
			if (--count==0) return;
		}
	}
}

/*==============================================================*
 *				revive	 			*
 *==============================================================*/
PUBLIC void revive(proc_nr,bytes)
int proc_nr;
int bytes;
{
	register struct fproc *rfp;
	register int task;

	if (proc_nr < 0 || proc_nr >= NR_PROCS) panic("revive err",proc_nr);
	rfp = &fproc[proc_nr];
	if (rfp->fp_suspended == NOT_SUSPENDED || rfp->fp_revived == REVIVING) return;
	
	task = -rfp->fp_task;
	if (task == XPIPE || task == XLOCK){
		rfp->fp_revived = REVIVING;
		reviving++;
	}else{
		rfp->fp_suspended = NOT_SUSPENDED;
		if (task == XPOPEN)
			reply(proc_nr,rfp->fp_fd>8);
		else{
			rfp->fp_nbytes = bytes;
			reply(proc_nr,bytes);
		}
	}
}

/*==============================================================*
 *				do_unpause	 		*
 *==============================================================*/
PUBLIC int do_unpause()
{
	register struct fproc *rfp;
	int proc_nr,task,fild;
	struct filp *f;
	dev_t dev;

	if (who > MM_PROC_NR) return (EPERM);
	proc_nr = pro;
	if (proc_nr < 0 || proc_nr >= NR_PROCS) panic("unpause err 1",proc_nr);
	rfp = &fproc[proc_nr];
	if (rfp->fp_suspended == NOT_SUSPENDED) return (OK);
	task = -rfp->fp_task;

	switch(task){
		case XPIPE:
			break;
		case XOPEN:
			panic("fs/do_unpause called with XOPEN\n",NO_NUM);
		case XLOCK:
			break;
		case XPOPEN:
			break;
		default:
			fild = (rfp->fp_fd >> 8) & BYTE;
			if (fild < 0 || fild >= OPEN_MAX) panic("unpause err 2",NO_NUM);
			f = rfp->fp_filp[fild];
			dev = (dev_t) f->filp_ino->i_zone[0];
			mess.TTY_LINE = (dev >> MINOR) & BYTE;
			mess.PROC_NR = proc_nr;

			mess.COUNT = (rfp->fp_fd & BYTE) == READ ? R_BIT : W_BIT;
			mess.m_type = CANCEL;
			fp = rfp;
			(*dmap[(dev >> MAJOR) & BYTE].dmap_rw)(task,&mess);
	}
	rfp->fp_suspended = NOT_SUSPENDED;
	reply(proc_nr,EINTR);
	return (OK);
}
