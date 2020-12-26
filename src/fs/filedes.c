


#include "fs.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"

/*==============================================================*
 *				get_fd				*
 *==============================================================*/
PUBLIC int get_fd(start,bits,k,fpt)
int start;
mode_t bits;
int *k;
struct filp **fpt;
{
	register struct filp *f;
	register int i;

	*k = -1;

	for (i=start;i<OPEN_MAX;i++){
		if (fp->fp_filp[i] == NIL_FILP){
			*k = i;
			break;
		}
	}

	if (*k < 0) return (EMFILE);

	for (f=&filp[0];f<&filp[NR_FILPS];f++){
		if (f->filp_count == 0){
			f->filp_mode = bits;
			f->filp_pos = 0L;
			f->filp_flags = 0;
			*fpt = f;
			return (OK);
		}
	}
	return (ENFILE);
}

/*==============================================================*
 *				get_filp			*
 *==============================================================*/
PUBLIC struct filp *get_filp(fild)
int fild;
{
	err_code = EBADF;
	if (fild < 0 || fild >= OPEN_MAX) return (NIL_FILP);
	return (fp->fp_filp[fild]);
}

/*==============================================================*
 *				find_filp			*
 *==============================================================*/
PUBLIC struct filp *find_filp(rip,bits)
register struct inode *rip;
Mode_t bits;
{
	register struct filp *f;
	
	for (f=&filp[0];f<&filp[NR_FILPS];f++){
		if (f->filp_count != 0 && f->filp_ino == rip && (f->filp_mode & bits)){
			return (f);
		}
	}
	return (NIL_FILP);
}
