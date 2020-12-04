
#include "mm.h"
#include <sys/stat.h>
#include <minix/callnr.h>
#include <a.out.h>
#include <string.h>
#include "mproc.h"
#include "param.h"

FORWARD _PROTOTYPE(void load_seg,(int fd,int seg,vir_bytes seg_bytes));
FORWARD _PROTOTYPE(int new_mem,(struct mproc *sh_mp,vir_bytes text_bytes,
			vir_bytes data_bytes,vir_bytes bss_bytes,
			vir_bytes stk_bytes,phys_bytes tot_bytes));
FORWARD _PROTOTYPE(void patch_ptr,(char stack [ARG_MAX],vir_bytes base));
FORWARD _PROTOTYPE(int read_header,(int fd,int *ft,vir_bytes *text_bytes,
			vir_bytes *data_bytes,vir_bytes *bss_bytes,
			phys_bytes *tot_bytes,long *sym_bytes,vir_clicks sc,
			vir_bytes *pc));

/*======================================================================*
 * 				do_exec					*
 *======================================================================*/
PUBLIC int do_exec()
{
	register struct mproc *rmp;
	struct mproc *sh_mp;
	int m,r,fd,ft,sn;
	static char mbuf[ARG_MAX];
	static char name_buf[PATH_MAX];
	char *new_sp,*basename;
	vir_bytes src,dst,text_bytes,data_bytes,bss_bytes,stk_bytes,vsp;
	phys_bytes tot_bytes;
	long sym_bytes;
	vir_clicks sc;
	struct stat s_buf;
	vir_bytes pc;

	rmp = mp;
	stk_bytes = (vir_bytes) stack_bytes;
	if (stk_bytes > ARG_MAX) return (ENOMEM);
	if (exec_len <= 0 || exec_len > PATH_MAX) return (EINVAL);
	
	src = (vir_bytes) exec_name;
	dst = (vir_bytes) name_buf;
	r = sys_copy(who,D,(phys_bytes) src,
			MM_PROC_NR,D,(phys_bytes) dst,(phys_bytes) exec_len);
	if (r != OK) return (r);
	tell_fs(CHDIR,who,FALSE,0);
	fd = allowed(name_buf,&s_buf,X_BIT);
	if (fd < 0) return (fd);

	sc = (stk_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	m = read_header(fd,&ft,&text_bytes,&data_bytes,&bss_bytes,&tot_bytes,&sym_bytes,sc,&pc);
	if (m<0){
		close(fd);
		return (ENOEXEC);
	}

	src = (vir_bytes) stack_ptr;
	dst = (vir_bytes) mbuf;
	r = sys_copy(who,D,(phys_bytes) src,
			MM_PROC_NR, D,(phys_bytes) dst,(phys_bytes) stk_bytes);
	if (r != OK){
		close(fd);
		return (EACCES);
	}

	sh_mp = find_share(rmp,s_buf.st_ino,s_buf.st_dev,s_buf.st_ctime);
	r = new_mem(sh_mp,text_bytes,data_bytes,bss_bytes,stk_bytes,tot_bytes);
	if (r != OK){
		close(fd);
		return (r);
	}

	rmp->mp_ino = s_buf.st_ino;
	rmp->mp_dev = s_buf.st_dev;
	rmp->mp_ctime = s_buf.st_ctime;

	vsp = (vir_bytes) rmp->mp_seg[S].mem_vir << CLICK_SHIFT;
	vsp += (vir_bytes) rmp->mp_seg[S].mem_len << CLICK_SHIFT;
	vsp -= stk_bytes;
	patch_ptr(mbuf,vsp);
	src = (vir_bytes) mbuf;
	r = sys_copy(MM_PROC_NR,D,(phys_bytes) src,who,D,(phys_bytes) vsp,(phys_bytes) stk_bytes);
	if (r!=OK) panic("do_exec stack copy err",NO_NUM);

	if (sh_mp != NULL){
		lseek(fd,(off_t) text_bytes,SEEK_CUR);
	}else{
		load_seg(fd,T,text_bytes);
	}
	load_seg(fd,D,data_bytes);

	close(fd);

	if ((rmp->mp_flags & TRACED) == 0){
		if (s_buf.st_mode & I_SET_UID_BIT){
			rmp->mp_effuid = s_buf.st_uid;
			tell_fs(SETUID,who,(int) rmp->mp_realuid,(int) rmp->mp_effuid);
		}
		if (s_buf.st_mode & I_SET_GID_BIT){
			rmp->mp_effgid = s_buf.st_gid;
			tell_fs(SETGID,who,(int) rmp->mp_readlgid,(int) rmp->mp_effgid);
		}
	}

	rmp->mp_procargs = vsp;

	for (sn=1;sn<=_NSIG;sn++){
		if (sigismember(&rmp->mp_catch,sn)){
			sigdelset(&rmp->mp_catch,sn);
			rmp->mp_sigact[sn].sa_handler = SIG_DFL;
			sigemptyset(&rmp->mp_sigact[sn].sa_mask);
		}
	}

	rmp->mp_flags &= ~SEPARATED;
	rmp->mp_flags |= ft;
	new_sp = (char *) vsp;

	tell_fs(EXEC,who,0,0);

	basename = strrchr(name_buf,'/');
	if (basename == NULL) basename = namebuf; else basename++;
	sys_exec(who,new_sp,rmp->mp_flags & TRACED,basename,pc);
	return (OK);
}

/*======================================================================*
 * 				read_header				*
 *======================================================================*/
PRIVATE int read_header(fd,ft,text_bytes,data_bytes,bss_bytes,tot_bytes,sym_bytes,sc,pc)
int fd;
int *ft;
vir_byte *text_bytes;
vir_byte *data_bytes;
vir_byte *bss_bytes;
vir_byte *tot_bytes;
long *sym_bytes;
vir_clicks sc;
vir_clicks *pc;
{
	int m,ct;
	vir_clicks tc,dc,s_vir,dvir;
	phys_clicks totc;
	struct exec hdr;

	if (read(fd,(char *) &hdr,A_MINHDR) != A_MINHDR) return (ENOEXEC);

	if (BADMAG(hdr)) return (ENOEXEC);
#if (CHIP == INTEL && _WORD_SIZE == 2)
	if (hdr.a_cpu != A_I8086) return (ENOEXEC);
#endif
#if (CHIP == INTEL && _WORD_SIZE == 4)
	if (hdr.a_cpu != A_I80386) return (ENOEXEC);
#endif
	if ((hdr.a_flags & ~(A_NSYM | A_EXEC | A_SEP)) != 0) return (ENOEXEC);

	*ft = ((hdr.a_flags & A_SEP) ? SEPARATE : 0);

	*text_bytes = (vir_bytes) hdr.a_text;
	*data_bytes = (vir_bytes) hdr.a_data;
	*bss_bytes = (vir_bytes) hdr.a_bss;
	*tot_bytes = hdr.a_total;
	*sym_bytes = hdr.a_sym;

	if (*tot_bytes == 0) return (ENOEXEC);

	if (*ft != SEPARATE){
		*data_bytes += *text_bytes;
		*text_bytes = 0;
	}

	*pc = hdr.a_entry;

	tc = ((unsigned long) *text_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	dc = (*data_bytes + *bss_bytes + CLICKS_SIZE - 1) >> CLICK_SHIFT;
	totc = (*tot_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	if (dc >= totc) return (ENOEXEC);

	dvir = (*ft == SEPARATE ? 0 : tc);
	s_vir = dvir + (totc - sc);
	m = size_of(*ft,tc,dc,sc,dvir,s_vir);
	ct = hdr.a_hdrlen & BYTE;
	if (ct > A_MINHDR) lseek(fd,(off_t) ct,SEEK_SET);
	return (m);
}

/*======================================================================*
 * 				new_mem					*
 *======================================================================*/
PRIVATE int new_mem(sh_mp,text_bytes,data_bytes,bss_bytes,stk_bytes,tot_bytes)
struct mproc *sh_mp;
vir_bytes text_bytes;
vir_bytes data_bytes;
vir_bytes bss_bytes;
vir_bytes stk_bytes;
vir_bytes tot_bytes;
{
	register sturct mproc *rmp;
	vir_clicks text_clicks,data_clicks,gap_clicks,stack_clicks,tot_clicks;
	phys_clicks new_base;

	static char zero[1024];
	phys_bytes bytes,base,count,bss_offset;

	if (sh_mp != NULL) text_bytes = 0;

	text_clicks = ((unsigned long) text_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	data_clicks = (data_bytes + bss_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	stack_clicks = (stk_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	tot_clicks = (tot_bytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	gap_clicks = tot_clicks - data_clicks - stack_clicks;
	if ((int) gap_clicks < 0) return (ENOMEM);
	
	if (text_clicks + tot_clicks > max_hole()) return (EAGAIN);

	rmp = mp;

	if (find_share(rmp,rmp->mp_ino,rmp->mp_dev,rmp->mp_ctime) == NULL){
		free_mem(rmp->mp_seg[T].mem_phys,rmp->mp_seg[T].mem_len);
	}

	free_mem(rmp->mp_seg[D].mem_phys,rmp->mp_seg[S].mem_vir + rmp->mp_seg[S].mem_len - rmp->mp_seg[D].mem_vir);

	new_base = alloc_mem(text_clicks + tot_clicks);
	if (new_base == NO_MEM) panic("MM hole list is inconsistent",NO_NUM);

	if (sh_mp != NULL){
		rmp->mp_seg[T] = sh_mp->mp_seg[T];
	}else{
		rmp->mp_seg[T].mem_phys = new_base;
		rmp->mp_seg[T].mem_vir = 0;
		rmp->mp_seg[T].mem_len = text_clicks;
	}
	rmp->mp_seg[D].mem_phys = new_base + text_clicks;
	rmp->mp_seg[D].mem_vir = 0;
	rmp->mp_seg[D].mem_len = data_clicks;
	rmp->mp_seg[S].mem_phys = rmp->mp_seg[D].mem_phys + data_clicks + gap_clicks;
	rmp->mp_seg[S].mem_vir = rmp->mp_seg[D].mem_vir + data_clicks + gap_clicks;
	rmp->mp_seg[D].mem_len = stack_clicks;

	sys_newmap(who,rmp->mp_seg);

	bytes = (phys_bytes)(data_clicks + gap_clicks + statck_clicks) << CLICK_SHIFT;
	base = (phys_bytes) rmp->mp_seg[D].mem_phys << CLICK_SHIFT;
	bss_offset = (data_bytes >> CLICK_SHIFT) << CLICK_SHIFT;
	base += bss_offset;
	bytes -= bss_offset;

	while(bytes>0){
		count = MIN(bytes,(phys_bytes) sizeof(zero));
		if (sys_copy(MM_PROC_NR,D,(phys_bytes) zero,
					ABS,0,base,count) != OK){
			panic("new_mem can't zero",NO_NUM);
		}
		base += count;
		bytes -= count;
	}
	return (OK);
}

/*======================================================================*
 * 				patch_ptr				*
 *======================================================================*/
PRIVATE void patch_ptr(stack,base)
char stack[ARG_MAX];
vir_bytes base;
{
	char **ap,flag;
	vir_bytes v;

	flag = 0;
	ap = (char **) stack;
	ap++;
	while(flag < 2){
		if (ap >= (char **) &stack[ARG_MAX]) return;
		if (*ap != NIL_PTR){
			v = (vir_bytes) *ap;
			v += base;
			*ap = (char *) v;
		}else{
			flag++;
		}
		ap++;
	}
}

/*======================================================================*
 * 				load_seg				*
 *======================================================================*/
PRIVATE void load_seg(fd,seg,seg_bytes)
int fd;
int seg;
vir_bytes seg_bytes;
{
	int new_fd,bytes;
	char *ubuf_ptr;

	new_fd = (who << 8) | (seg << 6) | fd;
	ubuf_ptr = (char *) ((vir_bytes)mp->mp_seg[seg].mem_vir << CLICK_SHIFT);
	while(seg_byte != 0){
		bytes = (INT_MAX / BLOCK_SIZE) * BLOCK_SIZE;
		if (seg_bytes < bytes)
			bytes = (int) seg_bytes;
		if (read(new_fd,ubuf_ptr,bytes) != bytes)
			break;
		ubuf_ptr += bytes;
		seg_bytes -= bytes;
	}
}

/*======================================================================*
 * 				find_share				*
 *======================================================================*/
PUBLIC struct mproc *find_share(mp_ign,ino,dev,ctime)
struct mproc *mp_ign;
ino_t ino;
dev_t dev;
time_t ctime;
{
	struct mproc *sh_mp;

	for (sh_mp=&mproc[INIT_PROC_NR];sh_mp < &mproc[NR_PROC];sh_mp++){
		if ((sh_mp->mp_flags & (IN_USE | HANGING | SEPARATE)) != (IN_USE | SEPARATE)) continue;
		if (sh_mp == mp_ign) continue;
		if (sh_mp->mp_ino != ino) continue;
		if (sh_mp->mp_dev != dev) continue;
		if (sh_mp->mp_ctime != ctime) continue;
		return sh_mp;
	}
	return (NULL);
}
