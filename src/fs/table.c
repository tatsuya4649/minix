
#define _TABLE

#include "fs.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include "buf.h"
#include "dev.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "lock.h"
#include "super.h"

PUBLIC _PROTOTYPE( int (*call_vector[NCALLS]),(void)) = {
	no_sys,
	do_exit,
	do_fork,
	do_read,
	do_write,
	do_open,
	do_close,
	no_sys,
	do_creat,
	do_link,
	do_unlink,
	no_sys,
	do_chdir,
	do_time,
	do_mknod,
	do_chmod,
	do_chown,
	no_sys,
	do_stat,
	do_lseek,
	no_sys,
	do_mount,
	do_umount,
	do_set,
	no_sys,
	do_stime,
	no_sys,
	no_sys,
	do_fstat,
	no_sys,
	do_utime,
	no_sys,
	no_sys,
	do_access,
	no_sys,
	no_sys,
	do_sync,
	no_sys,
	do_rename,
	do_mkdir,
	do_unlink,
	do_dup,
	do_pipe,
	do_tims,
	no_sys,
	no_sys,
	do_set,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	do_ioctl,
	do_fcntl,
	no_sys,
	no_sys,
	no_sys,
	do_exec,
	do_umask,
	do_chroot,
	do_setsid,
	no_sys,
	no_sys,
	do_unpause,
	no_sys,
	do_revive,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
	no_sys,
};

#define DT(enable,open,rw,close,task) \
	{ (enable ? (open) : no_dev),(enable ? (rw) : no_dev),\
		(enable ? (close) : no_dev),(enable ? (task) : 0)},

PUBLIC struct dmap dmap[] = {
	DT(1,no_dev,no_dev,no_dev,0)
	DT(1,dev_opcl,call_task,dev_opcl,MEM)
	DT(1,dev_opcl,call_task,dev_opcl,FLOPPY)
	DT(ENABLE_WINI,dev_opcl,call_task,dev_opcl,WINCHESTER)
	DT(1,tty_open,call_task,dev_opcl,TTY)
	DT(1,ctty_open,call_ctty,ctty_close,TTY)
	DT(1,dev_opcl,call_task,dev_opcl,PRINTER)
#if (MACHINE == IBM_PC)
	DT(ENABLE_NETWORKING,net_open,call_task,dev_opcl,INET_PROC_NR)
	DT(ENABLE_CDROM,dev_opcl,call_task,dev_opcl,CDROM)
	DT(0,0,0,0,0)
	DT(ENABLE_SCSI,dev_opcl,call_task,dev_opcl,SCSI)
	DT(0,0,0,0,0)
	DT(0,0,0,0,0)
	DT(ENABLE_AUDIO,dev_opcl,call_task,dev_opcl,AUDIO)
	DT(ENABLE_AUDIO,dev_opcl,call_task,dev_opcl,MIXER)
#endif
#if (MACHINE == ATARI)
	DT(ENABLE_SCSI,dev_opcl,call_task,dev_opcl,SCSI)
#endif
};

PUBLIC int max_major = sizeof(dmap)/sizeof(struct dmap);
