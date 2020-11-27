

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
