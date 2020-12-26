
#include "mm.h"
#include <sys/stat.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <fcntl.h>
#include <signal.h>
#include "mproc.h"

/*==============================================================*
 * 				allowed				*
 *==============================================================*/
PUBLIC int allowed(name_buf,s_buf,mask)
char *name_buf;
struct stat *s_buf;
int mask;
{
	int fd;
	int save_errno;

	if (access(name_buf,mask) < 0) return(-errno);

	tell_fs(SETUID,MM_PROC_NR,(int) SUPER_USER,(int) SUPER_USER);

	fd = open(name_buf,O_RDONLY);
	save_errno = errno;

	tell_fs(SETUID,MM_PROC_NR,(int) mp->mp_effuid,(int) mp->mp_effuid);
	if (fd<0) return (-save_errno);
	if (fstat(fd,s_buf) < 0) panic("allowed: fstat failed",NO_NUM);

	if (mask == X_BIT && (s_buf->st_mode & I_TYPE) != I_REGULAR){
		close(fd);
		return (EACCES);
	}
	return (fd);
}

/*==============================================================*
 * 				no_sys				*
 *==============================================================*/
PUBLIC int no_sys()
{
	return (EINVAL);
}

/*==============================================================*
 * 				panic				*
 *==============================================================*/
PUBLIC void panic(format,num)
char *format;
int num;
{
	printf("Memory manager panic: %s ",format);
	if (num != NO_NUM) printf("%d",num);
	printf("\n");
	tell_fs(SYNC,0,0,0);
	sys_abort(RBT_PANIC);
}

/*==============================================================*
 * 				tell_fs				*
 *==============================================================*/
PUBLIC void tell_fs(what,p1,p2,p3)
int what,p1,p2,p3;
{
	message m;
	m.m1_i1 = p1;
	m.m1_i2 = p2;
	m.m1_i3 = p3;
	_taskcall(FS_PROC_NR,what,&m);
}

