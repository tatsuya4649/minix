

#include "mm.h"
#include <sys/stat.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <sys/sigcontext.h>
#include <string.h>
#include "mproc.h"
#include "param.h"

#define CORE_MODE	0777
#define DUMPED		0200
#define DUMP_SIZE	((INT_MAX / BLOCK_SIZE) * BLOCK_SIZE)

FORWARD _PROTOTYPE( void check_pending,(void));
FORWARD _PROTOTYPE( void dump_core,(struct mproc *rmp));
FORWARD _PROTOTYPE( void unpause,(int pro));

/*======================================================================*
 *				do_sigaction				*
 *======================================================================*/
PUBLIC int do_sigaction()
{
	int r;
	struct sigaction svec;
	struct sigaction *svp;
	
	if (sig_nr == SIGKILL) return (OK);
	if (sig_nr < 1 || sig_nr > _NSIG) return (EINVAL);
	svp = &mp->mp_sigact[sig_nr];
	if ((struct sigaction *) sig_osa |= (struct sigaction *) NULL){
		r = sys_copy(MM_PROC_NR,D,(phys_bytes) svp,
				who,D,(phys_bytes) sig_osa,(phys_bytes) sizeof(svec));
		if (r != OK) return (r);
	}
	if ((struct sigaction *) sig_nsa == (struct sigaction *) NULL) return (OK);

	r = sys_copy(who,D,(phys_bytes) sig_nsa,
			MM_PROC_NR,D,(phys_bytes) &svec,(phys_bytes) sizeof(svec));
	if (r != OK) return (r);

	if (svec.sa_handler == SIG_IGN){
		sigaddset(&mp->mp_ignore,sig_nr);
		sigaddset(&mp->mp_sigpending,sig_nr);
		sigaddset(&mp->mp_catch,sig_nr);
	}else{
		sigdelset(&mp->mp_ignore,sig_nr);
		if (svec.sa_handler == SIG_DFL)
			sigdelset(&mp->mp_catch,sig_nr);
		else
			sigaddset(&mp->mp_catch,sig_nr);
	}
	mp->mp_sigact[sig_nr].sa_handler = svec.sa_handler;
	sigdelset(&svec.sa_mask,SIGKILL);
	mp->mp_sigact[sig_nr].sa_mask = svec.sa_mask;
	mp->mp_sigact[sig_nr].sa_flags = svec.sa_flags;
	mp->mp_sigreturn = (vir_bytes) sig_ret;
	return (OK);
}

/*======================================================================*
 *				do_sigpending				*
 *======================================================================*/
PUBLIC int do_sigpending()
{
	ret_mask = (long) mp->mp_sigpending;
	return OK;
}

/*======================================================================*
 *				do_sigprocmask				*
 *======================================================================*/
PUBLIC int do_sigprocmask()
{
	int i;

	ret_mask = (long) mp->mp_sigmask;

	switch(sig_how){
		case SIG_BLOCK:
			sigdelset((sigset_t *)&sig_set,SIGKILL);
			for (i=1;i<_NSIG;i++){
				if (sigismember((sigset_t *)&sig_set,i))
					sigaddset(&mp->mp_sigmask,i);
			}
			break;
		case SIG_UNBLOCK:
			for (i=1;i<_NSIG;i++){
				if (sigismember((sigset_t *)&sig_set,i))
					sigdelset(&mp->mp_sigmask,i);
			}
			check_pending();
			break;
		case SIG_SETMASK:
			sigdelset((sigset_t *) &sig_set,SIGKILL);
			mp->mp_sigmask = (sigset_t)sig_set;
			check_pending();
			break;
		case SIG_INQUIRE:
			break;
		default:
			return (EINVAL);
			break;
	}
	return (OK);
}

/*======================================================================*
 *				do_sigsuspend				*
 *======================================================================*/
PUBLIC int do_sigsuspend()
{
	mp->mp_sigmask2 = mp->mp_sigmask;
	mp->mp_sigmask = (sigset_t) sig_set;
	sigdelset(&mp->mp_sigmask,SIGKILL);
	mp->mp_flags |= SIGSUSPENDED;
	dont_reply = TRUE;
	check_pending();
	return OK;
}

/*======================================================================*
 *				do_sigreturn				*
 *======================================================================*/
PUBLIC int do_sigreturn()
{
	int r;
	mp->mp_sigmask = (sigset_t) sig_set;
	sigdelset(&mp->mp_sigmask,SIGKILL);
	r = sys_sigreturn(who,(vir_bytes) sig_context,sig_flags);
	check_pending();
	return (r);
}

/*======================================================================*
 *				do_kill					*
 *======================================================================*/
PUBLIC int do_kill()
{
	return check_sig(pid,sig_nr);
}

/*======================================================================*
 *				do_ksig					*
 *======================================================================*/
PUBLIC int do_ksig()
{
	register struct mproc *rmp;
	int i,proc_nr;
	pid_t proc_id,id;
	sigset_t sig_map;

	if (who!=HARDWARE) return (EPERM);
	dont_reply = TRUE;
	proc_nr = mm_in.SIG_PROC;
	rmp = &mproc[proc_nr];
	if ((rmp->mp_flags & IN_USE) == 0 || (rmp->mp_flags & HANGING)) return (OK);
	proc_id = rmp->mp_pid;
	sig_map = (sigset_t) mm_in.SIG_MAP;
	mp = &mproc[0];
	mp->mp_procgrp = rmp->mp_procgrp;
	for(i=1;i<=_NSIG;i++){
		if (!sigismember(&sig_map,i)) continue;
		switch(i){
			case SIGINT:
			case SIGQUIT:
				id = 0; break;
			case SIGKILL:
				id = -1; break;
			case SIGALRM:
				if ((rmp->mp_flags & ALARM_ON) == 0) continue;
				rmp->mp_flags &= ~ALARM_ON;
			default:
				id = proc_id;
				break;
		}
		check_sig(id,i);
		sys_endsig(proc_nr);
	}
	return (OK);
}

/*======================================================================*
 *				do_alarm				*
 *======================================================================*/
PUBLIC int do_alarm()
{
	return (set_alarm(who,seconds));
}

/*======================================================================*
 *				set_alarm				*
 *======================================================================*/
PUBLIC int set_alarm(proc_nr,sec)
int proc_nr;
int sec;
{
	message m_sig;
	int remaining;

	if (sec != 0)
		mproc[proc_nr].mp_flags |= ALARM_ON;
	else
		mproc[proc_nr].mp_flags &= ~ALARM_ON;
	
	m_sig.m_type = SET_ALARM;
	m_sig.CLOCK_PROC_NR = proc_nr;
	m_sig.DELTA_TICKS = (clock_t) (HZ * (unsigned long) (unsigned) sec);
	if ((unsigned long) m_sig.DELTA_TICKS / HZ != (unsigned) sec)
		m_sig.DELTA_TICKS = LONG_MAX;
	if (sendrec(CLOCK.&m_sig) != OK) panic("alarm er",NO_NUM);
	remaining = (int) m_sig.SECONDS_LEFT;
	if (remaining != m_sig.SECONDS_LEFT || remaining < 0)
		remaining = INT_MAX;
	return (remaining);
}

/*======================================================================*
 *				do_pause				*
 *======================================================================*/
PUBLIC int do_pause()
{
	mp->mp_flags |= PAUSED;
	dont_reply = TRUE;
	return (OK);
}

/*======================================================================*
 *				do_reboot				*
 *======================================================================*/
PUBLIC int do_reboot()
{
	register struct mproc *rmp = mp;
	char monitor_code[64];

	if (rmp->mp_effuid != SUPER_USER) return EPERM;

	switch(reboot_flag){
		case RBT_HALT:
		case RBT_REBOOT:
		case RBT_PANIC:
		case RBT_RESET:
			break;
		case RBT_MONITOR:
			if (reboot_size > sizeof(monitor_code)) return EINVAL;
			memset(monitor_code,0,sizeof(monitor_code));
			if (sys_copy(who,D,(phys_bytes) reboot_code,
						MM_PROC_NR,D,(phys_bytes) monitor_code,
						(phys_bytes) reboot_size) != OK) return EFAULT;
			if (monitor_code[sizeof(monitor_code)-1] != 0) return EINVAL;
			break;
		default:
			return EINVAL;
	}
	check_sig(-1,SIGKILL);

	tell_fs(EXIT,INIT_PROC_NR,0,0);

	tell_fs(SYNC,0,0,0);

	sys_abort(reboot_flag,monitor_code);
}

/*======================================================================*
 *				sig_proc				*
 *======================================================================*/
PUBLIC void sig_proc(rmp,signo)
register struct mproc *rmp;
int signo;
{
	vir_bytes new_sp;
	int slot;
	int sigflags;
	struct sigmsg sm;

	slot = (int) (rmp-mproc);
	if (!(rmp->mp_flags & IN_USE)){
		printf("MM: signal %d sent to dead process %d\n",signo,slot);
		panic("",NO_NUM);
	}
	if (rmp->mp_flags & HANGING){
		printf("MM: signal %d sent to HANGING process %d\n",signo,slot);
		panic("",NO_NUM);
	}
	if (rmp->mp_flags & TRACED && signo != SIGKILL){
		unpause(slot);
		stop_proc(rmp,signo);
		return;
	}
	if (sigismember(&rmp->mp_ignore,signo)) return;
	if (sigismember(&rmp->mp_sigmask,signo)){
		sigaddset(&rmp->mp_sigpending,signo);
		return;
	}
	sigflags = rmp->mp_sigact[signo].sa_flags;
	if (sigismember(&rmp->mp_catch,signo)){
		if (rmp->mp_flags & SIGSUSPENDED)
			sm.sm_mask = rmp->mp_sigmask2;
		else
			sm.sm_mask = rmp->mp_mask;
		sm.sm_signo = signo;
		sm.sm_sighandler = (vir_bytes) rmp->mp_sigact[signo].sa_handler;
		sys_getsp(slot,&new_sp);
		sm.sm_stkptr = new_sp;

		new_sp -= sizeof(struct sigcontext) + 3 * sizeof(char *) + 2 * sizeof(int);
		if (adjust(rmp,rmp->mp_seg[D].mem_len,new_sp)!=OK)
			goto doterminate;
		rmp->mp_sigmask |= rmp->mp_sigact[signo].sa_mask;
		if (sigflags & SA_NODEFER)
			sigdelset(&rmp->mp_sigmask,signo);
		else
			sigaddset(&rmp->mp_sigmask,signo);
		if (sigflags & SA_RESETHAND){
			sigdelset(&rmp->mp_catch,signo);
			rmp->mp_sigact[signo].sa_handler = SIG_DFL;
		}

		sys_sendsig(slot,&sm);
		sigdelset(&rmp->mp_sigpending,signo);
		unpause(slot);
		return;
	}
doterminate:
	rmp->mp_sigstatus = (char) signo;
	if (sigismember(&core_sset,signo)){
		tell_fs(CHDIR,slot,FALSE,0);
		dump_core(rmp);
	}
	mm_exit(rmp,0);
}

/*======================================================================*
 *				check_sig				*
 *======================================================================*/
PUBLIC int check_sig(proc_id,signo)
pid_t proc_id;
int signo
{
	register struct mproc *rmp;
	int count;
	int error_code;

	if (signo < 0 || signo > _NSIG) return (EINVAL);

	if (proc_id == INIT_PID && signo == SIGKILL) return (EINVAL);

	count = 0;
	error_code = ESRCH;
	for(rmp=&mproc[INIT_PROC_NR];rmp<&mproc[NR_PROCS];rmp++){
		if ((rmp->mp_flags & IN_USE) == 0) continue;
		if (rmp->mp_flags & HANGING && signo != 0) continue;

		if (proc_id > 0 && proc_id != rmp->mp_pid) continue;
		if (proc_id == 0 && mp->mp_procgrp != rmp->mp_procgrp) continue;
		if (proc_id == -1 && rmp->mp_pid == INIT_PID) continue;
		if (proc_id < -1 && rmp->mp_procgrp != -proc_id) continue;

		if (mp->mp_effuid != SUPER_USER
				&& mp->mp_realuid != rmp->mp_realuid
				&& mp->mp_effuid != rmp->mp_realuid
				&& mp->mp_realuid != rmp->mp_effuid
				&& mp->mp_effuid != rmp->mp_effuid){
			error_code = EPERM;
			continue;
		}
		count++;
		if (signo == 0) continue;

		if (proc_id > 0)break;
	}

	if ((mp->mp_flags & IN_USE) == 0 || (mp->mp_flgas & HANGING))
		dont_reply = TRUE;
	return (count > 0 ? OK : error_code);
}

/*======================================================================*
 *				check_pending				*
 *======================================================================*/
PRIVATE void check_pending()
{
	int i;
	for (i=1;i<_NSIG;i++){
		if (sigismember(&mp->mp_sigpending,i) &&
				!sigismember(&mp->mp_sigmask,i)){
			sigdelset(&mp->mp_sigpending,i);
			sig_proc(mp,i);
			break;
		}
	}
}

/*======================================================================*
 *				unpause					*
 *======================================================================*/
PRIVATE void unpause(pro)
int pro;
{
	register struct mproc *rmp;
	rmp = &mproc[pro];

	if ((rmp->mp_flags & PAUSED) && (rmp->mp_flags & HANGING) == 0){
		rmp->mp_flags &= ~PAUSED;
		reply(pro,EINTR,0,NIL_PTR);
		return;
	}

	if ((rmp->mp_flags & WAITING) && (rmp->mp_flags & HANGING) == 0){
		rmp->mp_flags &= ~WAITING;
		reply(pro,EINTR,0,NIL_PTR);
		return;
	}

	if ((rmp->mp_flags & SIGSUSPENDED) && (rmp->mp_flags & HANGING) == 0){
		rmp->mp_flags &= ~SIGSUSPENDED;
		reply(pro,EINTR,0,NIL_PTR);
		return;
	}
	tell_fs(UNPAUSE,pro,0,0);
}

/*======================================================================*
 *				dump_core				*
 *======================================================================*/
PRIVATE void dump_core(rmp)
register struct mproc *rmp;
{
	int fd,fake_fd,nr_written,seg,slot;
	char *buf;
	vir_bytes current_sp;
	phys_bytes left;
	unsigned nr_to_write;
	long trace_data,trace_off;

	slot = (int) (rmp - mproc);

	if (rmp->mp_realuid != rmp->mp_effuid) return;
	if ((fd = creat(core_name,CORE_MODE)) < 0) return;
	rmp->mp_sigstatus |= DUMPED;

	sys_getsp(slot,&current_sp);
	adjust(rmp,rmp->mp_seg[D].mem_len,current_sp);

	if (write(fd,(char *) rmp->mp_seg,(unsigned) sizeof rmp->mp_seg)
			!= (unsigned) sizeof rmp->mp_seg){
		close(fd);
		return;
	}

	trace_off = 0;
	while(sys_trace(3,slot,trace_off,&trace_data) == OK){
		if (write(fd,(char *) &trace_data,(unsigned) sizeof(long)) != (unsigned) sizeof (long)){
			close(fd);
			return;
		}
		trace_off += sizeof (long);
	}

	for(seg=0;seg<NR_SEGS;seg++){
		buf = (char *) ((vir_bytes) rmp->mp_seg[seg].mem_vir << CLICK_SHIFT);
		left = (phys_bytes) rmp->mp_seg[seg].mem_len << CLICK_SHIFT;
		fake_fd = (slot << 8) | (seg << 6) | fd;

		while (left != 0){
			nr_to_write = (unsigned) MIN(left,DUMP_SIZE);
			if ((nr_written = write(fake_fd,buf,nr_to_write)) < 0){
				close(fd);
				return;
			}
			buf += nr_written;
			left -= nr_written;
		}
	}
	close(fd);
}
