

#include "kernel.h"
#include <signal.h>
#include <unistd.h>
#include <sys/sigcontext.h>
#include <sys/ptrace.h>
#include <minix/boot.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "proc.h"
#include "protect.h"

#define IF_MASK		0x00000200
#define IOPL_MASK	0x003000

PRIVATE message m;

FORWARD _PROTOTYPE( int do_abort,(message *m_ptr));
FORWARD _PROTOTYPE( int do_copy,(message *m_ptr));
FORWARD _PROTOTYPE( int do_exec,(message *m_ptr));
FORWARD _PROTOTYPE( int do_fork,(message *m_ptr));
FORWARD _PROTOTYPE( int do_gboot,(message *m_ptr));
FORWARD _PROTOTYPE( int do_getsp,(message *m_ptr));
FORWARD _PROTOTYPE( int do_kill,(message *m_ptr));
FORWARD _PROTOTYPE( int do_mem,(message *m_ptr));
FORWARD _PROTOTYPE( int do_newmap,(message *m_ptr));
FORWARD _PROTOTYPE( int do_sendsig,(message *m_ptr));
FORWARD _PROTOTYPE( int do_sigreturn,(message *m_ptr));
FORWARD _PROTOTYPE( int do_endsig,(message *m_ptr));
FORWARD _PROTOTYPE( int do_times,(message *m_ptr));
FORWARD _PROTOTYPE( int do_trace,(message *m_ptr));
FORWARD _PROTOTYPE( int do_umap,(message *m_ptr));
FORWARD _PROTOTYPE( int do_xit,(message *m_ptr));
FORWARD _PROTOTYPE( int do_vcopy,(message *m_ptr));
FORWARD _PROTOTYPE( int do_getmap,(message *m_ptr));

/*=======================================================*
 * 			sys_task			*
 *=======================================================*/
PUBLIC void sys_task()
{
	register int r;
	
	while(TRUE){
		receive(ANY,&m);

		switch(m.m_type){
			case SYS_FORK:		r = do_fork(&m);	break;
			case SYS_NEWMAP:	r = do_newmap(&m);	break;
			case SYS_GETMAP:	r = do_getmap(&m);	break;
			case SYS_EXEC:		r = do_exec(&m);	break;
			case SYS_XIT:		r = do_xit(&m);		break;
			case SYS_GETSP:		r = do_getsp(&m);	break;
			case SYS_TIMES:		r = do_times(&m);	break;
			case SYS_ABORT:		r = do_abort(&m);	break;
			case SYS_SENDING:	r = do_sendsig(&m);	break;
			case SYS_SIGRETURN:	r = do_sigreturn(&m);	break;
			case SYS_KILL:		r = do_kill(&m);	break;
			case SYS_ENDSIG:	r = do_endsig(&m);	break;
			case SYS_COPY:		r = do_copy(&m);	break;
			case SYS_VCOPY:		r = do_vcopy(&m);	break;
			case SYS_GBOOT:		r = do_gboot(&m);	break;
			case SYS_MEM:		r = do_mem(&m);		break;
			case SYS_UMAP:		r = do_umap(&m);	break;
			case SYS_TRACE:		r = do_trace(&m);	break;
			default:		r = E_BAD_FCN;
		}

		m.m_type = r;
		send(m.m_source,&m);
	}
}

/*=======================================================*
 * 			do_fork				*
 *=======================================================*/
PRIVATE int do_fork(m_ptr)
register message *m_ptr;
{
	reg_t old_ldt_sel;
	register struct proc *rpc;
	struct proc *rpp;

	if (!isoksusern(m_ptr->PROC1) || !isoksusern(m_ptr->PROC2))
		return (E_BAD_PROC);
	rpp = proc_addr(m_ptr->PROC1);
	rpc = proc_addr(m_ptr->PROC2);

	old_ldt_sel = rpc->p_ldt_sel;

	*rpc = *rpp;

	rpc->p_ldt_sel = old_ldt_sel;
	rpc->p_nr = m_ptr->PROC2;

	rpc->p_flags |= NO_MAP;

	rpc->p_flags &= ~(PENDING | SIG_PENDING | P_STOP);

	sigemptyset(&rpc->p_pending);
	rpc->p_pendcount = 0;
	rpc->p_pid = m_ptr->PID;
	rpc->p_reg.retreg = 0;

	rpc->user_time = 0;
	rpc->sys_time = 0;
	rpc->child_utime = 0;
	rpc->child_stime = 0;

	return (OK);
}

/*=======================================================*
 * 			do_newmap			*
 *=======================================================*/
PRIVATE int do_newmap(m_ptr)
message *m_ptr;
{
	register struct proc *rp;
	phys_bytes src_phys;
	int caller;
	int k;
	int old_flags;
	struct mem_map *map_ptr;
	
	caller = m_ptr->m_source;
	k = m_ptr->PROC1;
	map_ptr = (struct mem_map *) m_ptr->MEM_PTR;
	if (!isokprocn(k)) return (E_BAD_PROC);
	rp = proc_addr(k);

	src_phys = umap(proc_addr(caller),D,(vir_bytes) map_ptr,sizeof(rp->p_map));
	if (src_phys == 0) panic("bad call to sys_newmap",NO_NUM);
	phys_copy(src_phys,vir2phys(rp->p_map),(phys_bytes) sizeof(rp->p_map));

	alloc_segments(rp);
	old_flags = rp->p_flags;
	rp->p_flags &= ~NO_MAP;
	if (old_flags != 0 && rp->p_flags == 0) lock_ready(rp);
	return (OK);
}

/*=======================================================*
 * 			do_getmap			*
 *=======================================================*/
PRIVATE int do_getmap(m_ptr)
message *m_ptr;
{
	register struct proc *rp;
	phys_bytes dst_phys;
	int caller;
	int k;
	struct mem_map *map_ptr;

	caller = m_ptr->m_source;
	k = m_ptr->PROC1;
	map_ptr = (struct mem_map *) m_ptr->MEM_PTR;

	if (!isokprocn(k))
		panic("do_getmap got bad proc: ",m_ptr->PROC1);
	
	rp = proc_addr(k);

	dst_phys = umap(proc_addr(caller),D,(vir_bytes) map_ptr,sizeof(rp->p_map));
	if (dst_phys == 0) panic("bad call to sys_getmap",NO_NUM);
	phys_copy(vir2phys(rp->p_map),dst_phys,sizeof(rp->p_map));
	return (OK);
}

/*=======================================================*
 * 			do_exec 			*
 *=======================================================*/
PRIVATE int do_exec(m_ptr)
register message *m_ptr;
{
	register struct proc *rp;
	reg_t sp;
	phys_bytes phys_name;
	char *np;
#define NLEN (sizeof(rp->p_name)-1)
	if (!isoksusern(m_ptr->PROC1)) return E_BAD_PROC;
	if (m_ptr->PROC2) cause_sig(m_ptr->PROC1,SIGTRAP);
	sp = (reg_t) m_ptr->STACK_PTR;
	rp = proc_addr(m_ptr->PROC1);
	rp->p_reg.sp = sp;
	rp->p_reg.pc = (reg_t) m_ptr->IP_PTR;
	rp->p_alarm = 0;
	rp->p_flags &= ~RECIVING;
	
	if (rp->p_flags == 0) lock_ready(rp);

	phys_name = numap(m_ptr->m_source,(vir_bytes) m_ptr->NAME_PTR,(vir_bytes) NLEN);

	if (phys_name != 0){
		phys_copy(phys_name, vir2phys(rp->p_name),(phys_bytes) NLEN);
		for (np=rp->p_name;(*np & BYTE) >= ' ';np++){}
		*np = 0;
	}
	return (OK);
}
/*=======================================================*
 * 			do_xit	 			*
 *=======================================================*/
PRIVATE int do_xit(m_ptr)
message *m_ptr;
{
	register struct proc *rp,*rc;
	struct proc *np,*xp;
	int parent;
	int proc_nr;
	phys_clicks base,size;

	parent = m_ptr->PROC1;
	proc_nr = m_ptr->PROC2;
	if (!isoksusern(parent) || !isoksusern(proc_nr)) return (E_BAD_PROC);
	rp = proc_addr(parent);
	rc = proc_addr(proc_nr);

	lock();
	rp->child_utime += rc->user_time + rc->child_utime;
	rp->child_stime += rp->sys_time + rc->child_stime;
	unlock();
	rp->p_alarm = 0;
	
	if (rc->p_flags == 0) lock_unready(rc);

	strcpy(rc->p_name,"<noname>");

	if (rc->p_flags & SENDING){
		for (rp = BEG_PROC_ADDR;rp<END_PROC_ADDR;rp++){
			if (rp->p_callerq == NIL_PROC) continue;
			if (rp->p_callerq == rc){
				rp->p_callerq = rc->p_sendlink;
				break;
			} else{
				np = rp->p_callerq;
				while( (xp=np->p_sendlink) != NIL_PROC)
					if (xp == rc){
						np->p_sendlink = xp->p_sendlink;
						break;
					}else{
						np = xp;
					}
			}
		}
	}

	if (rc->p_flags & PENDING) --sig_procs;
	sigemptyset(&rc->p_pending);
	rc->p_pendcount = 0;
	rc->p_flags = P_SLOT_FREE;
	return (OK);
}

/*=======================================================*
 * 			do_getsp	 		*
 *=======================================================*/
PRIVATE int do_getsp(m_ptr)
register message *m_ptr;
{
	register struct proc *rp;

	if (!isoksusern(m_ptr->PROC1)) return (E_BAD_PROC);
	rp = proc_addr(m_ptr->PROC1);
	m_ptr->STACK_PTR = (char *) rp->p_reg.sp;
	return (OK);
}
/*=======================================================*
 * 			do_times	 		*
 *=======================================================*/
PRIVATE int do_times(m_ptr)
register message *m_ptr;
{
	register struct proc *rp;
	
	if(!isoksusern(m_ptr->PROC1)) return E_BAD_PROC;
	rp = proc_addr(m_ptr->PROC1);
	
	lock();
	m_ptr->USER_TIME = rp->user_time;
	m_ptr->SYSTEM_TIME = rp->sys_time;
	unlock();
	m_ptr->CHILD_UTIME = rp->child_utime;
	m_ptr->CHILD_STIME = rp->child_stime;
	m_ptr->BOOT_TICKS = get_uptime();
	return (OK);
}
/*=======================================================*
 * 			do_abort	 		*
 *=======================================================*/
PRIVATE int do_abort(m_ptr)
message *m_ptr;
{
	char monitor_code[64];
	phys_bytes src_phys;

	if (m_ptr->m1_i1 == RBT_MONITOR){
		src_phys = numap(m_ptr->m_source,(vir_bytes) m_ptr->m1_p1,
				(vir_bytes) sizeof(monitor_code));
		if (src_phys == 0) panic("bad monitor code from",m_ptr->m_source);
		phys_copy(src_phys,vir2phys(monitor_code),
				(phys_bytes) sizeof(monitor_code));
		reboot_code = vir2phys(monitor_code);
	}
	wreboot(m_ptr->m1_i1);
	return(OK);
}
/*=======================================================*
 * 			do_sendsig	 		*
 *=======================================================*/
PRIVATE int do_sendsig(m_ptr)
message *m_ptr;
{
	struct sigmsg smsg;
	regsiter struct proc *rp;
	phys_bytes src_phys,dst_phys;
	struct sigcontext sc,*scp;
	struct sigframe fr,*frp;

	if (!isokusern(m_ptr->PROC1)) return (E_BAD_PROC);
	rp = proc_addr(m_ptr->PROC1);

	src_phys = umap(proc_addr(MM_PROC_NR),D,(vir_bytes) m_ptr->SIG_CTXT_PTR,
			(vir_bytes) sizeof(struct sigmsg));
	if (src_phys == 0)
		panic("do_sendsig can't signal: bad sigmsg address from MM",NO_NUM);
	phys_copy(src_phys,vir2phys(&smsg),(phys_bytes) sizeof(struct sigmsg));

	scp = (struct sigcontext *) smsg.sm_stkptr - 1;

	memcpy(&sc.sc_regs,&rp->p_reg,sizeof(struct sigregs));

	sc.sc_flags = SC_SIGCONTEXT;

	sc.sc_mask = smsg.sm_mask;

	dst_phys = umap(rp,D,(vir_bytes) scp,
			(vir_bytes) sizeof(struct sigcontext));
	if (dst_phys == 0) return (EFAULT);
	phys_copy(vir2phys(&sc),dst_phys,(phys_bytes) sizeof(struct sizecontext));

	frp = (struct sigframe *) scp - 1;
	fr.sf_scpcopy = scp;
	fr.sf_retadr2 = (void (*) ()) rp->p_reg.pc;
	fr.sf_fp = rp->p_reg.fp;
	rp->p_reg.fp = (reg_t) &frp->sf_fp;
	fr.sf_scp = scp;
	fr.sf_code = 0;
	fr.sf_signo = smsg.sm_signo;
	fr.sg_retadr = (void (*)()) smsg.sm_sigreturn;

	dst_phys = umap(rp,D,(vir_bytes) frp,(vir_bytes) sizeof(struct sigframe));
	if (dst_phys == 0) return(EFAULT);
	phys_copy(vir2phys(&fr),dst_phys,(phys_bytes) sizeof(struct sigframe));

	rp->p_reg.sp = (reg_t) frp;
	rp->p_reg.pc = (reg_t) smsg.sm_sighandler;
	return (OK);
}

/*=======================================================*
 * 			do_sigreturn 			*
 *=======================================================*/
PRIVATE int do_sigreturn(m_ptr)
register message *m_ptr;
{
	struct sigcontext sc;
	register struct proc *rp;
	phys_bytes src_phys;

	if (!isokusern(m_ptr->PROC1)) return(E_BAD_PROC);
	rp = proc_addr(m_ptr->PROC1);

	src_phys = umap(rp,D,(vir_bytes) m_ptr->SIG_CTXT_PTR,
			(vir_bytes) sizeof(struct sigcontext));
	if (src_phys == 0) return (EFAULT);
	phys_copy(src_phys,vir2phys(&sc),(phys_bytes) sizeof(struct sigcontext));

	if ((sc.sc_flags & SC_SIGCONTEXT) == 0) return (EINVAL);

	if (sc.sc_flags & SC_NOREGLOCALS){
		rp->p_reg.retreg = sc.sc_retreg;
		rp->p_reg.fp = sc.sc_fp;
		rp->p_reg.pc = sc.sc_pc;
		rp->p_reg.sp = sc.sc_sp;
		return(OK);
	}
	sc.sc_psw = rp->p_reg.psw;

#if (CHIP==INTEL)
	sc.sc_cs = rp->p_reg.cs;
	sc.sc_ds = rp->p_reg.ds;
	sc.sc_es = rp->p_reg.es;
#if _WORD_SIZE == 4
	sc.sc_fs = rp->p_reg.fs;
	sc.sc_gs = rp->p_reg.gs;
#endif
#endif

	memcpy(&rp->p_reg,(char *)&sc.sc_regs,sizeof(struct sigregs));
	return (OK);
}
/*=======================================================*
 * 			do_endsig 			*
 *=======================================================*/
PRIVATE int do_endsig(m_ptr)
register message *m_ptr;
{
	register struct proc *rp;

	if (!isokusern(m_ptr->PROC1)) return(E_BAD_PROC);
	rp = proc_addr(m_ptr->PROC1);

	if (rp->p_pendcount != 0 && --rp->p_pendcount == 0
			&& (rp->p_flags &= ~SIG_PENDING) == 0)
		lock_ready(rp);
	return (OK);
}

/*=======================================================*
 * 			do_copy 			*
 *=======================================================*/
PRIVATE int do_copy(m_ptr)
register message *m_ptr;
{
	int src_proc,dst_proc,src_space,dst_space;
	vir_bytes src_vir,dst_vir;
	phys_bytes src_phys,dst_phys,bytes;

	src_proc = m_ptr->SRC_PROC_NR;
	dst_proc = m_ptr->DST_PROC_NR;
	src_space = m_ptr->SRC_SPACE;
	dst_space = m_ptr->DST_SPACE;
	src_vir = (vir_bytes) m_ptr->SRC_BUFFER;
	dst_vir = (vir_bytes) m_ptr->DST_BUFFER;
	bytes = (phys_bytes) m_ptr->COPY_BYTES;

	if (src_proc == ABS)
		src_phys = (phys_bytes) m_ptr->SRC_BUFFER;
	else{
		if (bytes != (vir_bytes) bytes)
			panic("overflow in count in do_copy",NO_NUM);
		src_phys = umap(proc_addr(src_proc), src_space,src_vir,(vir_bytes) bytes);
	}
	if (dst_proc == ABS)
		dst_phys = (phys_bytes) m_ptr->DST_BUFFER;
	else
		dst_phys = umap(proc_addr(dst_proc),dst_space,dst_vir,(vir_bytes) bytes);

	if (src_phys == 0 || dst_phys == 0) return (EFAULT);
	phys_copy(src_phys,dst_phys,bytes);
	return (OK);
}

/*=======================================================*
 * 			do_vcopy 			*
 *=======================================================*/
PRIVATE int do_vcopy(m_ptr)
register message *m_ptr;
{
	int src_proc,dst_proc,vect_s,i;
	vir_bytes src_vir,dst_vir,vect_addr;
	phys_bytes src_phys,dst_phys,bytes;
	cpvec_t cpvec_table[CPVEC_NR];

	src_proc = m_ptr->m1_i1;
	dst_proc = m_ptr->m1_i2;
	vect_s = m_ptr->m1_i3;
	vect_addr = (vir_bytes)m_ptr->m1_p1;

	if (vect_s > CPVEC_NR) return EDOM;

	src_phys = numap(m_ptr->m_source,vect_addr,vect_s * sizeof(cpvec_t));
	if (!src_phys) return EFAULT;
	phys_copy(src_phys,vir2phys(cpvec_table),(phys_bytes) (vect_s * sizeof(cpvec_t)));
	for (i=0;i<vect_s;i++){
		src_vir = cpvec_table[i].cpv_src;
		dst_vir = cpvec_table[i].cpv_dst;
		bytes = cpvec_table[i].cpv_size;
		src_phys = numap(src_proc.src_vir,(vir_bytes) bytes);
		dst_phys = numap(dst_proc.dst_vir,(vir_bytes) bytes);
		if (src_phys==0 || dst_phys==0) return (EFAULT);
		phys_copy(src_phys,dst_phys,bytes);
	}
	return (OK);
}
/*=======================================================*
 * 			do_gboot 			*
 *=======================================================*/
PUBLIC struct bparam_s boot_parameters;

PRIVATE int do_gboot(m_ptr)
message *m_ptr;
{
	phys_bytes dst_phys;

	dst_phys = umap(proc_addr(m_ptr->PROC1),D,(vir_bytes) m_ptr->MEM_PTR,
			(vir_bytes) sizeof(boot_parameters));
	if (dst_phys==0) panic("bad call to SYS_GBOOT",NO_NUM);
	phys_copy(vir2phys(&boot_parameters),dst_phys,(phys_bytes) sizeof(boot_parameters));
	return (OK);
}
/*=======================================================*
 * 			do_mem				*
 *=======================================================*/
PRIVATE int do_mem(m_ptr)
register message *m_ptr;
{
	struct memory *memp;

	for (memp=mem;memp<&mem[NR_MEMS];++memp){
		m_ptr->m1_i1 = memp->base;
		m_ptr->m1_i2 = memp->size;
		m_ptr->m1_i3 = tot_mem_size;
		memp->size = 0;
		if (m_ptr->m1_i2 != 0) break;
	}
	return (OK);
}

/*=======================================================*
 * 			do_umap				*
 *=======================================================*/
PRIVATE int do_umap(m_ptr)
register message *m_ptr;
{
	m_ptr->SRC_BUFFER = umap(proc_addr((int) m_ptr->SRC_PROC_NR),
			(int) m_ptr->SRC_SPACE,
			(vir_bytes) m_ptr->SRC_BUFFER,
			(vir_bytes) m_ptr->COPY_BYTES);
	return (OK);
}

/*=======================================================*
 * 			do_trace			*
 *=======================================================*/
#define TR_PROCNR
#define TR_REQUEST
#define TR_ADDR
#define TR_DATA
#define TR_VLSIZE

PRIVATE int do_trace(m_ptr)
register message *m_ptr;
{
	register struct proc *rp;
	phys_bytes src,dst;
	int i;

	rp = proc_addr(TR_PROCNR);
	if (rp->p_flags & P_SLOT_FREE) return (EIO);
	switch(TR_REQUEST){
		case T_STOP:
			if (rp->p_flags == 0) lock_unready(rp);
			rp->p_flags |= P_STOP;
			rp->p_reg.psw &= ~TRACEBIT;
			return (OK);
		case T_GETINS:
			if (rp->p_map[T].mem_len != 0){
				if ((src=umap(rp,T,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
				phys_copy(src,vir2phys(&TR_DATA),(phys_bytes) sizeof(long));
				break;
			}
		case T_GETDATA:
			if ((src = umap(rp,D,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
			phys_copy(src,vir2phys(&TR_DATA),(phys_bytes) sizeof(long));
			break;
		case T_GETUSER:
			if ((TR_ADDR & (sizeof(long) - 1)) != 0 || TR_ADDR>sizeof(struct proc) - sizeof(long))
				return (EIO);
			TR_DATA = *(long *) ((char *) rp + (int) TR_ADDR);
			break;
		case T_SETINS:
			if (rp->p_map[T].mem_len != 0){
				if ((dst = umap(rp,T,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
				phys_copy(vir2phys(&TR_DATA),dst,(phys_bytes) sizeof(long));
				break;
			}
		case T_SETDATA:
			if ((dst = umap(rp,D,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
			phys_copy(vir2phys(&TR_DATA),dst,(phys_bytes) sizeof(long));
			TR_DATA = 0;
			break;
		case T_SETUSER:
			if ((TR_ADDR & (sizeof(reg_t) - 1)) != 0 || TR_ADDR > sizeof(struct stackframe_s) - sizeof(reg_t))
				return (EIO);
			i = (int) TR_ADDR;
#if (CHIP == INTEL)
			if (i==(int) &((struct proc *) 0)->p_reg.cs ||
					i == (int) &((struct proc *) 0)->p_reg.ds ||
					i == (int) &((struct proc *) 0)->p_reg.es ||
#if _WORD_SIZE == 4
					i == (int) &((struct proc *) 0)->p_reg.gs ||
					i == (int) &((struct proc *) 0)->p_reg.fs ||
#endif
					i == (int) &((struct proc *) 0)->p_reg.ss)
				return (EIO);
#endif
			if (i==(int) &((struct proc *) 0)->p_reg.psw)
				SETPSW(rp,TR_DATA);
			else
				*(reg_t *) ((char *) &rp->p_reg + i) = (reg_t) TR_DATA;
			TR_DATA = 0;
			break;
		case T_RESUME:
			rp->p_flags &= ~P_STOP;
			if (rp->p_flags == 0) lock_ready(rp);
			TR_DATA = 0;
			break;
		case T_STEP:
			rp->p_reg.psw |= TRACEBIT;
			rp->p_flag &= ~P_STOP;
			if (rp->p_flags == 0) lock_ready(rp);
			TR_DATA = 0;
			break;
		default:
			return (EIO);
	}
	return (OK);
}
#define TR_PROCNR
#define TR_REQUEST
#define TR_ADDR
#define TR_DATA
#define TR_VLSIZE

PRIVATE int do_trace(m_ptr)
register message *m_ptr;
{
	register struct proc *rp;
	phys_bytes src,dst;
	int i;

	rp = proc_addr(TR_PROCNR);
	if (rp->p_flags & P_SLOT_FREE) return (EIO);
	switch(TR_REQUEST){
		case T_STOP:
			if (rp->p_flags == 0) lock_unready(rp);
			rp->p_flags |= P_STOP;
			rp->p_reg.psw &= ~TRACEBIT;
			return (OK);
		case T_GETINS:
			if (rp->p_map[T].mem_len != 0){
				if ((src=umap(rp,T,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
				phys_copy(src,vir2phys(&TR_DATA),(phys_bytes) sizeof(long));
				break;
			}
		case T_GETDATA:
			if ((src = umap(rp,D,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
			phys_copy(src,vir2phys(&TR_DATA),(phys_bytes) sizeof(long));
			break;
		case T_GETUSER:
			if ((TR_ADDR & (sizeof(long) - 1)) != 0 || TR_ADDR>sizeof(struct proc) - sizeof(long))
				return (EIO);
			TR_DATA = *(long *) ((char *) rp + (int) TR_ADDR);
			break;
		case T_SETINS:
			if (rp->p_map[T].mem_len != 0){
				if ((dst = umap(rp,T,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
				phys_copy(vir2phys(&TR_DATA),dst,(phys_bytes) sizeof(long));
				break;
			}
		case T_SETDATA:
			if ((dst = umap(rp,D,TR_ADDR,TR_VLSIZE)) == 0) return (EIO);
			phys_copy(vir2phys(&TR_DATA),dst,(phys_bytes) sizeof(long));
			TR_DATA = 0;
			break;
		case T_SETUSER:
			if ((TR_ADDR & (sizeof(reg_t) - 1)) != 0 || TR_ADDR > sizeof(struct stackframe_s) - sizeof(reg_t))
				return (EIO);
			i = (int) TR_ADDR;
#if (CHIP == INTEL)
			if (i==(int) &((struct proc *) 0)->p_reg.cs ||
					i == (int) &((struct proc *) 0)->p_reg.ds ||
					i == (int) &((struct proc *) 0)->p_reg.es ||
#if _WORD_SIZE == 4
					i == (int) &((struct proc *) 0)->p_reg.gs ||
					i == (int) &((struct proc *) 0)->p_reg.fs ||
#endif
					i == (int) &((struct proc *) 0)->p_reg.ss)
				return (EIO);
#endif
			if (i==(int) &((struct proc *) 0)->p_reg.psw)
				SETPSW(rp,TR_DATA);
			else
				*(reg_t *) ((char *) &rp->p_reg + i) = (reg_t) TR_DATA;
			TR_DATA = 0;
			break;
		case T_RESUME:
			rp->p_flags &= ~P_STOP;
			if (rp->p_flags == 0) lock_ready(rp);
			TR_DATA = 0;
			break;
		case T_STEP:
			rp->p_reg.psw |= TRACEBIT;
			rp->p_flag &= ~P_STOP;
			if (rp->p_flags == 0) lock_ready(rp);
			TR_DATA = 0;
			break;
		default:
			return (EIO);
	}
	return (OK);
}

/*=======================================================*
 * 			cause_sig			*
 *=======================================================*/
PUBLIC void cause_sig(proc_nr,sig_nr)
int proc_nr;
int sig_nr;
{
	register struct proc *rp,*mmp;

	rp = proc_addr(proc_nr);
	if (sigismemver(&rp->p_pending,sig_nr))
		return;
	sigaddset(&rp->p_pending,sig_nr);
	++rp->p_pendcount;
	if (rp->p_flags & PENDING)
		return;
	if (rp->p_flags == 0) lock_unready(rp);
	rp->p_flags |= PENDING | SIG_PENDING;
	++sig_procs;

	mmp = proc_addr(MM_PROC_NR);
	if ( ((mmp->p_flags & RECEIVING) == 0) || mmp->p_getfrom != ANY) return;
	inform();
}

/*=======================================================*
 * 			inform				*
 *=======================================================*/
PUBLIC void inform()
{
	register struct proc *rp;

	for (rp=BEG_SERV_ADDR; rp<END_PROC_ADDR;rp++)
		if (rp->p_flags & PENDING){
			m.m_type = KSIG;
			m.SIG_PROC = proc_number(rp);
			m.SIG_MAP = rp->p_pending;
			sig_procs--;
			if (lock_mini_send(proc_addr(HARDWARE),MM_PROC_NR,&m) != OK)
				panic("can't inform MM",NO_NUM);
			sigemptyset(&rp->p_pending);
			rp->p_flags &= ~PENDING;
			lock_pick_proc();
			return;
		}
}

/*=======================================================*
 * 			umap				*
 *=======================================================*/
PUBLIC phys_bytes umap(rp,seg,vir_addr,bytes)
register struct proc *rp;
int seg;
vir_bytes vir_addr;
vir_bytes bytes;
{
	vir_clicks vc;
	phys_bytes pa;
	phys_bytes seg_base;

	if(bytes <= 0) return ((phys_bytes)0);
	vc = (vir_addr + bytes - 1) >> CLICK_SHIFT;

	if (seg != T)
		seg = (vc < rp->p_map[D].mem_vir + rp->p_map[D].mem_len ? D : S);

	if ((vir_addr>>CLICK_SHIFT) >= rp->p_map[seg].mem_vir+rp->p_map[seg].mem_len)
		return ((phys_bytes) 0);
	seg_base = (phys_bytes) rp->p_map[seg].mem_phys;
	seg_base = seg_base << CLICK_SHIFT;
	pa = (phys_bytes) vir_addr;
	pa -= rp->p_map[seg].mem_vir << CLICK_SHIFT;
	return (seg_base + pa);
}

/*=======================================================*
 * 			numap				*
 *=======================================================*/
PUBLIC phys_bytes numap(proc_nr,vir_addr,bytes)
int proc_nr;
vir_bytes vir_afddr;
vir_bytes bytes;
{
	return (umap(proc_addr(proc_nr),D,vir_addr,bytes));
}

/*=======================================================*
 * 			alloc_segments			*
 *=======================================================*/
PUBLIC void alloc_segments(rp)
register struct proc *rp;
{
	phys_bytes code_bytes;
	phys_bytes data_bytes;
	int privilege;

	if (protected_mode){
		data_bytes = (phys_bytes) (rp->p_map[S].mem_vir + rp->p_map[S].mem_len)
			<< CLICK_SHIFT;
		if (rp->p_map[T].mem_len == 0)
			code_bytes = data_bytes;
		else
			code_bytes = (phys_bytes) rp->p_map[T].mem_len << CLICK_SHIFT;
		privilege = istaskp(rp) ? TASK_PRIVILEGE : USER_PRIVILEGE;
		init_codeseg(&rp->p_ldt[CS_LDT_INDEX],
				(phys_bytes) rp->p_map[T].mem_phys << CLICK_SHIFT,
				code_bytes,privilege);
		init_dataseg(&rp->p_ldt[DS_LDT_INDEX],
				(phys_bytes) rp->p_map[D].mem_phys << CLICK_SHIFT,
				data_bytes,privilege);
		rp->p_reg.cs = (CS_LDT_INDEX * DESC_SIZE) | TI | privilege;
#if _WORD_SIZE == 4
		rp->p_reg.gs =
		rp->p_reg.fs =
#endif
		rp->p_reg.ss = 
		rp->p_reg.es = 
		rp->p_reg.ds = (DS_LDT_INDEX * DESC_SIZE) | TI | privilege;
	}else{
		rp->p_reg.cs = click_to_hclick(rp->p_map[T].mem_phys);
		rp->p_reg.ss = 
		rp->p_reg.es = 
		rp->p_reg.ds = click_to_hclick(rp->p_map[D].mem_phys);
	}
}
#endif // CHIP == INTEL
