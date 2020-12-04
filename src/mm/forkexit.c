
#include "mm.h"
#include <sys/wait.h>
#include <minix/callnr.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

#define LAST_FEW		2

PRIVATE pid_t next_pid = INIT_PID + 1;

FORWARD _PROTOTYPE(void cleanup,(register struct mproc *child));

/*==============================================================*
 * 				do_fork				*
 *==============================================================*/
PUBLIC int do_fork()
{
	register struct mproc *rmp;
	register struct mproc *rmc;
	int i,child_nr,t;
	phys_clicks prog_clicks,child_base = 0;
	phys_bytes prog_bytes,parent_abs,child_abs;

	rmp = mp;
	if (procs_in_use == NR_PROCS) return (EAGAIN);
	if (procs_in_use >= NR_PROCS-LAST_FEW && rmp->mp_effuid != 0) return (EAGAIN);

	prog_clicks = (phys_clicks) rmp->mp_seg[S].mem_len;
	prog_clicks += (rmp->mp_seg[S].mem_vir - rmp->mp_seg[D].mem_vir);
	prog_bytes = (phys_bytes) prog_clicks << CLICK_SHIFT;
	if ( (child_base = alloc_mem(prog_clicks)) == NO_MEM) return (EAGAIN);

	child_abs = (phys_bytes) child_base << CLICK_SHIFT;
	parent_abs = (phys_bytes) rmp->mp_seg[D].mem_phys << CLICK_SHIFT;
	i = sys_copy(ABS,0,parent_abs,ABS,0,child_abs,prog_bytes);
	if (i<0) panic("do_fork can't copy",i);

	for (rmc = &mproc[0]; rmc < &mproc[NR_PROCS]; rmc++)
		if ((rmc->mp_flags & IN_USE) == 0) break;

	child_nr = (int)(rmc - mproc);
	procs_in_use++;
	*rmc = *rmp;

	rmc->mp_parent = who;
	rmc->mp_flags &= ~TRACED;

	if (!(rmc->mp_flags & SEPARATE)) rmc->mp_seg[T].mem_phys = child_base;
	rmc->mp_seg[D].mem_phys = child_base;
	rmc->mp_seg[S].mem_phys = rmc->mp_seg[D].mem_phys + 
		(rmp->mp_seg[S].mem_vir - rmp->mp_seg[D].mem_vir);
	rmc->mp_exitstatus = 0;
	rmc->mp_sigstatus = 0;

	do{
		t = 0;
		next_pid = (next_pid < 30000 ? next_pid + 1 : INIT_PID + 1);
		for (rmp=&mproc[0];rmp<&mproc[NR_PROC];rmp++)
			if (rmp->mp_pid==next_pid || rmp->mp_procgrp==next_pid){
				t = 1;
				break;
			}
		rmc->mp_pid = next_pid;
	}while(t);

	sys_fork(who,child_nr,rmc->mp_pid,child_base);
	tell_fs(FORK,who,child_nr,rmc->mp_pid);

	sys_newmap(child_nr,rmc->mp_seg);

	reply(child_nr,0,0,NIL_PTR);
	return (next_pid);
}

/*==============================================================*
 * 				do_exit				*
 *==============================================================*/
PUBLIC int do_mm_exit()
{
	mm_exit(mp,status);
	dont_reply = TRUE;
	return (OK);
}

/*==============================================================*
 * 				mm_exit				*
 *==============================================================*/
PUBLIC void mm_exit(rpm,exit_status)
register struct mproc *rmp;
int exit_status;
{
	register int proc_nr;
	int parent_waiting,right_child;
	pid_t pidarg,procgrp;
	phys_clicks base,size,s;

	proc_nr = (int) (rmp-mproc);

	procgrp = (rmp->mp_pid == mp->mp_procgrp) ? mp->mp_procgrp : 0;

	if (rmp->mp_flags & ALARM_ON) set_alarm(proc_nr,(unsigned) 0);
	
	tell_fs(EXIT,proc_nr,0,0);
	
	sys_xit(rmp->mp_parent,proc_nr,&base,&size);

	if (find_share(rmp,rmp->mp_ino,rmp->mp_dev,rmp->mp_ctime) == NULL){
		free_mem(rmp->mp_seg[T].mem_phys,rmp->mp_seg[T].mem_len);
	}
	free_mem(rmp->mp_seg[D].mem_phys,rmp->mp_seg[S].mem_vir+rmp->mp_seg[S].mem_len - rmp->mp_seg[D].mem_vir);

	rmp->mp_exitstatus = (char) exit_status;
	pidarg = mproc[rmp->mp_parent].mp_wpid;
	parent_waiting = mproc[rmp->mp_parent].mp_flags & WAITING;
	if (pidarg == -1 || pidarg == rmp->mp_pid || -pidarg == rmp->mp_procgrp)
		right_child = TRUE;
	else
		right_child = FALSE;
	if (parent_waiting && right_child)
		cleanup(rmp);
	else
		rmp->mp_flags |= HANGING;

	for (rmp = &mproc[0];rmp<&mproc[NR_PROCS];rmp++){
		if (rmp->mp_flags & IN_USE && rmp->mp_parent==proc_nr){
			rmp->mp_parent = INIT_PROC_NR;
			parent_waiting = mproc[INIT_PROC_NR].mp_flags & WAITING;
			if (parent_waiting && (rmp->mp_flags & HANGING)) cleanup(rmp);
		}
	}

	if (procgrp != 0) check_sig(-procgrp,SIGHUP);
}

/*==============================================================*
 * 				do_waitpid			*
 *==============================================================*/
PUBLIC int do_waitpid()
{
	register struct mproc *rp;
	int pidarg,options,children,res2;

	pidarg = (mm_call == WAIT ? -1 : pid);
	options = (mm_call == WAIT ? 0 : sig_nr);
	if (pidarg == 0) pidarg = -mp->mp_procgrp;

	children = 0;
	for (rp = &mproc[0];rp<&mproc[NR_PROCS];rp++){
		if ((rp->mp_flags & IN_USE) && rp->mp_parent == who){
			if (pidarg > 0 && pidarg != rp->mp_pid) continue;
			if (pidarg < -1 && -pidarg != rp->mp_procgrp) continue;

			children++;
			if (rp->mp_flags & HANGING){
				cleanup(rp);
				dont_reply = TRUE;
				return (OK);
			}
			if ((rp->mp_flags & STOPPED) && rp->mp_sigstatus){
				res2 = 0177 | (rp->mp_sigstatus << 8);
				reply(who,rp->mp_pid,res2,NIL_PTR);
				dont_reply = TRUE;
				rp->mp_sigstatus = 0;
				return (OK);
			}
		}
	}

	if (children > 0){
		if (options & WNOHANG) return (0);
		mp->mp_flgas |= WAITING;
		mp->mp_wpid = (pid_t) pidarg;
		dont_reply = TRUE;
		return (OK);
	}else{
		return (ECHILD);
	}
}

/*==============================================================*
 * 				cleanup				*
 *==============================================================*/
PRIVATE void cleanup(child)
register struct mproc *child;
{
	int exitstatus;

	exitstatus = (child->mp_exitstatus << 8) | (child->mp_sigstatus & 0377);
	reply(child->mp_parent,child->mp_pid,exitstatus,NIL_PTR);
	mproc[child->mp_parent].mp_flags &= ~WAITING;

	child->mp_flags = 0;
	peocs_in_use--;
}
