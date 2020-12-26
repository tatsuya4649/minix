
#include "fs.h"
#include <fcntl.h>
#include <unistd.h>
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "lock.h"
#include "param.h"

/*======================================================*
 * 			lock_op				*
 *======================================================*/
PUBLIC int lock_op(f,req)
struct filp *f;
int req;
{
	int r,ltype,i,conflict=0,unlocking=0;
	mode_t mo;
	off_t first,last;
	struct flock flock;
	vir_bytes user_flock;
	struct file_lock *flp,*flp2,*empty;

	user_flock = (vir_bytes) name1;
	r = sys_copy(who,D,(phys_bytes) user_flock,
			FS_PROC_NR,D,(phys_bytes)  &flock,(phys_bytes) sizeof(flock));
	if (r!=OK) return (EINVAL);

	ltype = flock.l_type;
	mo = f->filp_mode;
	if (ltype != F_UNLCK && ltype != F_RDLCK && ltype != F_WRLCK) return (EINVAL);
	if (req == F_GETCK && ltype == F_UNLCK) return (EINVAL);
	if ((f->filp_ino->i_mode & I_TYPE) != I_REGULAR) return (EINVAL);
	if (req != F_GETLK && ltype == F_RDLCK && (mo & R_BIT) == 0) return (EBADF);
	if (req != F_GETLK && ltype == F_WELCK && (mo & W_BIT) == 0) return (EBADF);

	switch(flock.l_whence){
		case SEEK_SET:	first = 0; break;
		case SEEK_CUR:	first = f->filp_pos; break;
		case SEEK_END:	first = f->filp_ino->i_size; break;
		default:	return (EINVAL);
	}
	if (((long) flock.l_start > 0) && ((first + flock.l_start) < first))
		return (EINVAL);
	if (((long) flock.l_start < 0) && ((first + flock.l_start) > first))
		return (EINVAL);
	first = first + flock.l_start;
	last = first + flock.l_len - 1;
	if (flock.l_len == 0) last = MAX_FILE_POS;
	if (last < first) return (EINVAL);

	empty = (struct file_lock *) 0;
	for (flp = &file_lock[0]; flp<&file_lock[NR_LOCKS];flp++){
		if (flp->lock_type == 0){
			if (empty == (struct file_lock *) 0) empty = flp;
			continue;
		}
		if (flp->lock_inode != f->filp_ino) continue;
		if (last < flp->lock_first) continue;
		if (first > flp->lock_last) continue;
		if (ltype == F_RDLCK && flp->lock_type == F_RDLCK) continue;
		if (ltype != F_UNLCK && flp->lock_pid == fp->fp_pid) continue;

		conflict = 1;
		if (req == F_GETLK) break;

		if (ltype == F_RDLCK || ltype == F_WRLCK){
			if (req == F_SETLK){
				return (EAGAIN);
			}else{
				suspend(XLOCK);
				return(0);
			}
		}
		
		unlocking = 1;
		if (first <= flp->lock_first && last >= flp->lock_last){
			flp->lock_type = 0;
			nr_locks--;
			continue;
		}

		if (first <= flp->lock_first){
			flp->lock_first = last + 1;
			continue;
		}

		if (last >= flp->lock_last){
			flp->lock_last = first - 1;
			continue;
		}

		if (nr_locks == NR_LOCKS) return (ENOLCK);
		for (i=0;i<NR_LOCKS;i++)
			if (file_lock[i].lock_type == 0) break;

		flp2 = &file_lock[i];
		flp2->lock_type = flp->lock_type;
		flp2->lock_pid = flp->lock_pid;
		flp2->lock_inode = flp->lock_inode;
		flp2->lock_first = flp->lock_first;
		flp2->lock_last = flp->lock_last;
		flp->lock_last = first - 1;
		nr_locks++;
	}
	if (unlocking) lock_revive();

	if (req == F_GETLK){
		if (conflict){
			flock.l_type = flp->lock_type;
			flock.l_whence = SEEK_SET;
			flock.l_start = flp->lock_first;
			flock.l_len = flp->lock_last - flp->lock_first + 1;
			flock.l_pid = flp->lock_pid;
		} else{
			flock.l_type = F_UNLCK;
		}

		r = sys_copy(FS_PROC_NR,0,(phys_bytes) &flock,
				who,D,(phys_bytes) user_flock,(phys_bytes) sizeof(flock));
		return (r);
	}

	if (ltype == F_UNLCK) return (OK);

	if (empty == (struct file_lock *) 0) return (ENOLCK);
	empty->lock_type = ltype;
	empty->lock_pid = fp->fp_pid;
	empty->lock_inode = f->filp_ino;
	empty->lock_first = first;
	empty->lock_last = last;
	nr_locks++;
	return (OK);
}

/*======================================================*
 * 			lock_revive			*
 *======================================================*/
PUBLIC void lock_revive()
{
	int task;
	struct fproc *fptr;

	for (fptr = &fproc[INIT_PROC_NR+1];fptr<&fproc[NR_PROCS];fptr++){
		task -= -fptr->fp_task;
		if (fptr->fp_susupend == SUSPEND && task == XLOCK){
			revive( (int) (fptr - fproc),0);
		}
	}
}
