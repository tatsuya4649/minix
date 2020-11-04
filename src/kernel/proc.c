// this file contains essentially all of the process and message processing.
// from the outside there are two main entry pointers.
// 	
// 	sys_call  : called when a process or task performs SEND,RECEIVE,or SENDREC
// 	interrupt : called by an interrupt routine to send a message to a task
//
// there are also some minor entry points:
//
// 	lock_ready : put the process in one of the pending queues so that it can run
// 	lock_unready : exclude processes from the run town queue
// 	lock_sched : process execution was too long : schedule another process
// 	lock_mini_send : send message ( used by interrupt signals etc. ) 
// 	lock_pick_proc : choose a process to run ( used by system initialization )
// 	unhold : repeat all held interrupts
//

#include "kernel.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include "proc.h"

PRIVATE unsigned char switching;			// if non-zero,prohibit interrupt()

FORWARD _PROTOTYPE( int mini_send,(struct proc *caller_ptr,int dest,message *m_ptr));
FORWARD _PROTOTYPE( int mini_rec,(struct proc *caller_ptr,int src,message *m_ptr));
FORWARD _PROTOTYPE( void ready, (struct proc *rp));
FORWARD _PROTOTYPE( void sched,(void));
FORWARD _PROTOTYPE( void unready, (struct proc *rp));
FORWARD _PROTOTYPE( void pick_proc, (void));

#define CopyMess(s,sp,sm,dp,dm) cp_mess(s,(sp)->p_map[D].mem_phys,(vir_bytes)sm,(dp)->p_map[0],mem_phys,(vir_bytes)dm)

/*======================================================================================*
 * 					interrupt					*
 *======================================================================================*/
PUBLIC void interrupt(task)
int task;				// number of tasks started
{
	// an interrupt has occurred.schedule the task to handle it.
	register struct proc *rp;			// pointer to the process entry of the task
	rp = proc_addr(task);

	// if this call conflicts with another process change function,put it in the "held"
	// queue that will be flushed with the next non-conflict restart().
	// the race conditions are as follows : 
	// (1) k_reenter == (typeof k_reenter) -1 :
	// 	called from the task level,usually from the output interrupt routine.
	// 	it is possible to re-enter interrupt() from the interrupt handler.
	// 	since it is raer,no special treatment is required.
	// (2) k_reenter > 0 :
	// 	a call from a nested interrupt handler. the previous interrupt handler may
	// 	be inside interrupt() or sys_call().
	// (3) switching != 0 
	// 	some process switching function other than interrupt() is called from
	// 	the task level(usually from CLOCK to sched()).
	// 	an interrupt handler may call an interrupt and pass the k_reenter test.
	// 	
	if (k_reenter != 0 || switching){
		lock();
		if (!re->p_int_held){
			rp->p_int_held = TRUEl
			if (held_head != NIL_PROC)
				held_tail->p_nextheld = rp;
			else
				held_head = rp;
			held_tail = rp;
			rp->p_nextheld = NIL_PROC;
		}
		unlock();
		return;
	}
	
	// log a block if the task is not waiting for an interrupt
	if ( (rp->p_flags & (RECEIVING | SENDING)) != RECEIVING || !isrxhandware(rp->p_getfrom)){
		rp->p_int_blocked = TRUE;
		return;
	}

	// the destination is waiting for an interrupt.
	// HARDWARE sends it the message HARD_INT.
	// interrupt messages are not queued,so it is impossible to provide reliable information
	// about their anomalies.
	rp->p_messbuf->m_source = HARDWARE;
	rp->p_messbug->m_type = HARD_INT;
	rp->p_flags &= RECEIVING;
	rp->p_int_blocked = FALSE;

	// prepare rp and run rp unless the task is already running.
	// this will be ready(rp) inline in terms of speed.
	if (rdy_head[TASK_Q] != NIL_PROC)
		rdy_tail[TASK_Q]->p_nextready = rp;
	else
		proc_ptr = rdy_head[TASK_Q] = rp;
	rdy_tail[TASK_Q] = rp;
	rp->p_nextready = NIL_PROC;
}

/*======================================================================================*
 * 					sys_call					*
 *======================================================================================*/
PUBLIC int sys_call(function,src_dest,m_ptr)
int function;				// SEND,RECEIVEE,or both
int src_dest;				// source of sending of source of receiving
message *m_ptr;				// pointer to the message
{
	// the only system call in minix is sending and receiving message.
	// these are done by trapping in the kernle with the INT instruction.
	// the trap is captured and sys_call() is called to send and / or receive the message.
	// the caller is given by proc_ptr.
	register struct proc *rp;
	int n;
	// check for incorrect system call parameters
	if (!isoksrc_dest(src_dest)) return (E_BAD_SRC);
	rp = proc_ptr;
	
	if (isuserp(rp) && function != BOTH) return (E_NO_PERM);

	// the parameter are normal,so make a call
	if (function & SEND){
		// function=SEND or BOTH
		n = mini_send(rp,src_dest,m_ptr);
		if (function == SEND || n != OK)
			return(n);		// complete or SEND failed
	}

	// function = RECEIVE or BOTH
	return (mini_rec(rp,src_dest,m_ptr));


/*======================================================================================*
 * 					mini_send					*
 *======================================================================================*/
PRIVATE int mini_send(caller_ptr,dest,m_ptr)
register struct proc *caller_ptr;		// who is trying to send a message
int dest;					// who is receiving the message
message *m_ptr;					// pointer to message buffer
{
	// send a message from 'caller_ptr' to 'dest'.if the destination blocks waiting
	// for this message, copy the message to it and unblock the destination.
	// if the destination is not waiting at all,or is waiting for another source,
	// queue to 'caleler_ptr'
	
	register struct proc *dest_ptr,*next_ptr;
	vir_bytes vb;				// vir_bytes : message buffer pointer
	vir_clicks vlo,vhi;			// virtual click containing outgoing message

	// user process are only allowed to send to FS and MM.check this.
	if (isuserp(caller_ptr) && !issysentn(dest)) return (E_BAD_DEST);
      	dest_ptr = proc_addr(dest);		// pointer to process entry of destination
	if (dest_ptr->p_flags & P_SLOT_FREE) return (E_BAD_DEST);	// destination is dead
	
	// the check allows message to be placed anywhere in the data,
	// stack or gap. it will have to be improved later for machine with unmapped gaps.
	vb = (vir_bytes) m_ptr;
	vlo = vb >> CLICK_SHIFT;		// greatly vir click at the end of the message
	vhi = (vb + MESS_SIZE - 1) >> CLICK_SHIFT;	// click vir for the beginning of the message
	if (vlo < caller_ptr->p_map[D].mem_vir || vlo > vhi || vhi >= caller_ptr->p_map[S].mem_vir + caller_ptr->p_map[S].mem_len)
		return (EFAULT);

	// check deadlocks due to 'caller_ptr' and 'dest' sending to each other
	if (dest_ptr->p_flags & SENDING){
		next_ptr = proc_addr(dest_ptr->p_sendto);
		while(TRUE){
			if (next_ptr == caller_ptr) return(ELOCKED);
			if (next_ptr->p_flags & SENDING)
				next_ptr = proc_addr(next_ptr->p_sendto);
			else
				break;
		}
	}

	// check if the destination is blocked waiting for this message
	if ( (dest_ptr->p_flags & (RECEIVING || SENDING)) == RECEIVING && (dest_ptr->p_getfrom == ANY || dest_ptr->p_getfrom == proc_number(caller_ptr)) ) {
		// the destination is actually waiting for this message
		CopyMess(proc_number(caller_ptr),caller_ptr,m_ptr,dest_ptr,dest_ptr->p_messbuf);
		dest_ptr->p_flags &= RECEIVING;		// unblock destinations
		if (dest_ptr->p_flags == 0) ready(dest_ptr);
	}else{
		// the destination is not waiting.block and queue the caller
		caller_ptr->p_messbuf = m_ptr;
		if (caller_ptr->p_flags == 0) unready(caller_ptr);
		caller_ptr->p_flags |= SENDING;
		caller_ptr->p_sendto = dest;
		
		// now that the process is blocked,put it in the destination queue
		if ( (next_ptr = dest_ptr->p_callerq) == NIL_PROC)
			dest_ptr->p_callerq = caller_ptr;
		else{
			while(next_ptr->p_sendlink != NIL_PROC)
				next_ptr = next_ptr->p_sendlink;
			next_ptr->p_sendlink = caller_ptr;
		}
		caller_ptr->p_sendlink = NIL_PROC;
	}
	return(OK);
}
/*======================================================================================*
 * 					mini_rec					*
 *======================================================================================*/
PRIVATE int mini_rec(caller_ptr,src,m_ptr)
register struct proc *caller_ptr;		// process is trying to get the message
int src;					// which message source you need (or ANY)
message *m_ptr;					// pointer to message buffer
{
	// the process or task wants to get the message. if the message is already queued,
	// get it and unlock the sender.
	// block the caller if a message from the desired source is unavailable.there is no
	// need to check the validity of the parameters. user calls are always made at sendrec()
	// and checked by mini_send() . calls from tasks such as MM and FS can be trusted.
	register struct proc *sender_ptr;
	register struct proc *previous_ptr;

	// check if message from the desired source are already available
	if (!(caller_ptr->p_flags & SENDING)){
		// check caller queue
		for (sender_ptr=caller_ptr->p_callerq;sender_ptr!=NIL_PROC;
				previous_ptr=sender_ptr,sender_ptr=sender_ptr->p_sendlink){
			if(src==ANY||src==proc_number(sender_ptr)){
				// discover acceptable message
				CopyMess(proc_number(sender_ptr),sender_ptr,sender_ptr->p_messbuf,caller_ptr,m_ptr);
				if(sender_ptr==caller_ptr->p_callerq)
					caller_ptr->p_callerq = sender_ptr->p_sendelink;
				else
					previous_ptr->p_sendlink = sender_ptr->p_sendlink;
				if((sender_ptr->p_flagss &= ~SENDING) == 0)
						ready(sender_ptr);	// unblock the sender
				return (OK);
			}	
		}
	}
	// check for blocked interrupts
	if (caller_ptr->p_int_blocked && isrxhardware(src)){
		m_ptr->m_source = HARDWARE;
		m_ptr->m_type = HARD_INT;
		caller_ptr->p_int_blocked = FALSE;
		return (OK);
	}
	// if no suitable message exists,block the process trying to receive it
	caller_ptr->p_getfrom = src;
	caller_ptr->p_messbuf = m_ptr;
	if(caller_ptr->p_flags == 0) unready(caller_ptr);
	caller_ptr->p_flags |= RECEIVING;

	// if the MM has just blocked and there is an unannounced kernel signal,
	// the MM can accept the message and inform the MM of it.
	if(sig_procs > 0 && proc_number(caller_ptr) == MM_PROC_NR && src == ANY)
		inform();
	return(OK);
}


/*======================================================================================*
 * 					pick_proc					*
 *======================================================================================*/
PRIVATE void pick_proc()
{
	// decide who should do it now. the new process is selected by setting 'proc_ptr'.
	// if you choose a new user( or idle) process.record it in 'bill_ptr' so that
	// the clock task decides who to charge for system time.
	register struct proc *rp;		// process to be executed
	
	if( (rp=rdy_head[TASK_Q]) != NIL_PROC){
		proc_ptr = rp;
		return;
	}
	if( (rp=rdy_head[SERVER_Q]) != NIL_PROC){
		proc_ptr = rp;
		return;
	}
	if( (rp=rdy_head[USER_Q]) != NIL_PROC){
		proc_ptr = rp;
		bill_ptr = rp;
		return;
	}
	// if none of the tasks are ready,run an idle task.
	// to avoid this special case,the user task is always prepared creates an idle task.
	bill_ptr = proc_ptr = proc_addr(IDLE);
}

/*======================================================================================*
 * 					ready 						*
 *======================================================================================*/
PRIVATE void ready(rp)
register struct proc *rp;			// this process is currently executable
{
	// add 'rp' to one of the queue of executable processes.
	// the following three queues are prepared : 
	// 	* TASK_Q --- (highest priority) for executable tasks
	// 	* SERVER_Q --- (intermediate priority) for MM and FS only
	// 	* USER_Q --- (lowest priority) for user process
	
	if(istaskp(rp)){
		if (rdy_head[TASK_Q] != NIL_PROC)
			// add to the end of a non-empty queue
			rdy_tail[TASK_Q]->p_nextready = rp;
		else{
			proc_ptr = 		// run fresh task next
			rdy_head[TASK_Q] = rp;	// add to empty queue
		}
		rdy_tail[TASK_Q] = rp;
		rp->p_nextready = NIL_PROC;	// new enrey has no successor
		return;
	}
	if (!isuserp(rp)){	// others are similar
		if (rdy_head[SERVER_Q] != NIL_PROC)
			rdy_tail[SERVER_Q]->p_nextready = rp;
		else
			rdy_head[SERVER_Q] = rp;
		rdy_tail[SERVER_Q] = rp;
		rp->p_nextready = NIL_PROC;
		return;
	}
	if (rdy_head[USER_Q] != NIL_PROC)
		rdy_tail[USER_Q] = rp;
	rp->p_nextready = rdy_head[USER_Q];
	rdy_head[USER_Q] = rp;
	/*
	if (rdy_head[USER_Q] != NIL_PROC)
	 	rdy_tail[USER_Q]->p_nextready = rp;
	else
		rdy_head[USER_Q] = rp;
	rdy_tail[USER_Q] = rp;
	rp->p_nextready = NIL_PROC;
	*/


/*======================================================================================*
 * 					unready 					*
 *======================================================================================*/
PRIVATE void unready(rp)
register struct proc *rp;			// this process is no longer runnable
{
	// a process has blocked
	register struct proc *xp;
	register struct proc **qtail;		// TAKS_Q,SERVER_Q, or USER_Q rdy_tail

	if (istaskp(rp)){
		// task stack still ok?
		if (*rp->p_stguard != STACK_GUARD)
			panic("stack overrun by task",proc_number(rp));
		
		if ((xp = rdy_head[TASK_Q]) == NIL_PROC) return;
		if (xp == rp){
			// remove head of queue
			rdy_head[TASK_Q] = xp->p_nextready;
			if(rp == proc_ptr) pick_proc();
		}
		qtail = &rdy_tail[TASK_Q];
	}
	else if (!isuserp(rp)){
		if((xp=rdy_head[SERVER_Q]) == NIL_PROC) return;
		if(xp==rp){
			rdy_head[SERVER_Q] = xp->p_nextready;
			pick_proc();
			return;
		}
		qtail = &rdy_tail[SERVER_Q];
	}else
	{
		if ((xp = rdy_head[USER_Q]) == NIL_PROC) return;
		if (xp == rp){
			rdy_head[USER_Q] = xp->p_nextready;
			pick_proc();
			return;
		}
		qtail = &rdy_tail[USER_Q];
	}

	// serarch body of queue. A process can be made unready even if it is
	// not running by being sent a signal that kills it.
	while(xp->p_nextready != rp)
		if ((xp=xp->p_nextready) == NIL_PROC) return;
	xp->p_nextready = xp->p_nextready->p_nextready;
	if (*qtail == rp) *qtail = xp;

/*======================================================================================*
 * 					sched	 					*
 *======================================================================================*/
PRIVATE void sched()
{
	// the current process has run too long.if another low priority (user) process is
	// runnable,put the current process on the end of the user queue,
	// possible promoting another user to head of the queue.
	if (rdy_head[USER_Q] == NIL_PROC) return;

	// one or more user process queued
	rdy_tail[USER_Q]->p_nextready = rdy_head[USER_Q];
	rdy_tail[USER_Q] = rdy_head[USER_Q];
	rdy_tail[USER_Q] = rdy_head[USER_Q]->p_nextready;
	rdy_tail[USER_Q]->p_nextready = NIL_PROC;
	pick_proc();
}

/*======================================================================================*
 * 					lock_mini_send					*
 *======================================================================================*/
PUBLIC int lock_mini_send(caller_ptr,dest,m_ptr)
struct proc *caller_ptr;		// who is trying to send a message?
int dest;				// to whom is message being sent?
message *m_ptr;				// pointer to message buffer
{
	// safe gateway to mini_send() for tasks.
	int result;

	switching = TRUE;
	result = mini_send(caller_ptr,dest,m_ptr);
	switching = FALASE;
	return (result);
}

/*======================================================================================*
 * 					lock_pick_proc					*
 *======================================================================================*/
PUBLIC void lock_pick_proc()
{
	// safe gateway to pick_proc() for tasks.
	switching = TRUE;
	pick_proc();
	switching = FALSE;
}

/*======================================================================================*
 * 					lock_ready					*
 *======================================================================================*/
PUBLIC void lock_ready(rp)
struct proc *rp;			// this process is now runnable
{
	// sage gateway to ready() for tasks.
	switching = TRUE;
      	ready(rp);
	switching = FALSE;
}


/*======================================================================================*
 * 					lock_unready					*
 *======================================================================================*/
PUBLIC void lock_unready(rp)
struct proc *rp;			// this process is no longer runnable
{
	// sage gateway to unready() for tasks.
	switching = TRUE;
	unready(rp);
	switching = FALSE;
}

/*======================================================================================*
 * 					lock_sched					*
 *======================================================================================*/
PUBLIC void lock_sched()
{
	// safe gateway to sched() for tasks.
	switching = TRUE;
	sched();
	switching = FALSE;
}

/*======================================================================================*
 * 					unhold						*
 *======================================================================================*/
PUBLIC	void unhold()
{
	// flush any held-up interrupts.k_reenter must be 0.held_head must not be NIL_PROC.
	// interrupts must be disabled.they will be enabled but will be disabled when
	// this returns.
	register struct proc *rp;		// current head of held queue

	if (switching) return;
	re = held_head;
	do{
		if ((held_head = rp->p_nextheld)==NIL_PROC) held_tail = NIL_PROC;
		rp->p_int_held = FALSE;
		unlock();			// reduce latency; held queue may change
		interrupt(proc_number(rp));
		lock();				// protect the held queue again
	}
	while( (rp=held_head) != NIL_PROC);
}

