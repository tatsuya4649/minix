
#include "mm.h"
#include <sys/ptrace.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

#define NIL_MPROC		((struct mproc *) 0)

FORWARD _PROTOTYPE( struct mproc *findproc,(pid_t lpid));

/*==============================================================*
 * 				do_trace			*
 *==============================================================*/
PUBLIC int do_trace()
{
	register struct mproc *child;
	
	if (request == T_OK){
		mp->mp_flags |= TRACED;
		mm_out.m2_l2 = 0;
		return (OK);
	}
	if ((child=findproc(pid)) == NIL_MPROC || !(child->mp_flags & STOPPED)){
		return (ESRCH);
	}
	switch(request){
		case T_EXIT:
			mm_exit(child,(int) data);
			mm_out.m2_l2 = 0;
			return(OK);
		case T_RESUME:
		case T_STEP:
			if (data < 0 || data > _NSIG) return(EIO);
			if (data > 0){
				child->mp_flags &= ~TRACED;
				sig_proc(child,(int) data);
				child->mp_flags |= TRACED;
			}
			child->mp_flags &= ~STOPPED;
			break;
	}
	if (sys_trace(request,(int) (child - mproc), taddr,&data) != OK)
		return (-errno);
	mm_out.m2_l2 = data;
	return (OK);
}

/*==============================================================*
 * 				findproc			*
 *==============================================================*/
PRIVATE struct mproc *findproc(lpid)
pid_t lpid;
{
	register struct mproc *rmp;

	for(rmp=&mproc[INIT_PROC_NR+1];rmp<&mproc[NR_PROCS];rmp++)
		if (rmp->mp_flags & IN_USE && rmp->mp_pid == lpid) return (rmp);
	return(NIL_MPROC);
}

/*==============================================================*
 * 				stop_proc			*
 *==============================================================*/
PUBLIC void stop_proc(rmp,signo)
register struct mproc *rmp;
int signo;
{
	register struct mproc *rpmp = mproc + rmp->mp_parent;

	if (sys_trace(-1,(int)(rmp-mproc),0L,(long *) 0) != OK) return;
	rmp->mp_flags |= STOPPED;
	if (rpmp->mp_flags & WAITING){
		rpmp->mp_flags &= ~WAITING;
		reply(rmp->mp_parent,rmp->mp_pid,0177 | (signo << 8),NIL_PTR);
	}else{
		rmp->mp_sigstatus = signo;
	}
	return;
}
