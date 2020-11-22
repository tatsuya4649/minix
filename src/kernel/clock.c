/* this file contains the code and data for the clock task.
*  the clock task accept six message type:
*
* 	HARD_INT:	a clock interrupt has occured
*	GET_UPTIME:	get the time since boot in ticks
* 	GET_TIME:	a process wants the real time in seconds
* 	SET_TIME:	a process wants to set the real time in seconds
* 	SET_ALARM:	a process wants to be alerted after a specified interval
* 	SET_SYN_AL:	set the sync alarm
*
*	
* the input message is format m6. the parameters are as follows:
*
* 	m_type		CLOCK_PROC		FUNC		NEW_TIME
* ----------------------------------------------------------------------------
* |	HARD_INT    |			|		   |		     |
* |-----------------+-------------------+------------------+-----------------+
* |   GET_UPTIME    |			|		   |		     |
* |-----------------+-------------------+------------------+-----------------+
* |   GET_TIME      |			|		   |		     |
* |-----------------+-------------------+------------------+-----------------+
* |   SET_TIME      |			|		   |	newtime	     |
* |-----------------+-------------------+------------------+-----------------+
* |   SET_ALARM     |	proc_nr		|  f to call	   |	delta	     |
* |-----------------+-------------------+------------------+-----------------+
* |   SET_SYN_AL    |	proc_nr		|		   |    delta	     |
* |-----------------+-------------------+------------------+-----------------+
*
* NEW_TIME,DELTA_CLICKS, and SECONDS_LEFT all refer to the same field in the
* message,depending upon the message type.
*
* reply message are of type OK,except in the case fo a HARD_INT,to
* which no reply is generated.for the GET_* messages the time is returned
* in the NEW_TIME field,and for the SET_ALARM and SET_SYNC_AL the time
* in seconds remaining until the alarm is returned is returned in the same
* field.
* 
* when an alarm geos off,if the caller is a user process,a SIGALARM signal
* is sent to it.if it is a task,a function specified by the caller will
* be invoked. this function may,for example,send a message,but only if
* it is certain that the task will be blocked when the timer goes off.
* a synchronous alarm sends a message to the synchronous alarm task,
* which in turn can dispatch a message to another server.this is the only
* way to send an alarm to a server,since servers cannot use the function-call
* mechanism available to tasks and servers cannot receive signals. 
*
*/

#include "kernel.h"
#include <signal.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "proc.h"

// constant definitions
#define MILLISEC		100	// how often to call the scheduler (msec)
#define SCHED_RATE	(MILLISEC*HZ/1000)	// number of ticks per schedule

// clock parameters
#if (CHIP == INTEL)
#define COUNTER_FREQ	(2*TIMER_FREQ)	// counter frequency using square wave
#define LATCH_COUNT	0x00		// cc00xxxx, c = channel,x = any
#define SQUARE_WAVE	0x36		// ccaammmb,a=access,m=mode,b=BCD
					// 11x11,11=LSB then MSB,x11=sq wave
#define TIMER_COUNT	((unsigned) (TIMER_FREQ/HZ))	// initial value of counter
#define TIMER_FREQ	1193182L	// clock frequency for timer in PC/AT
#define CLOCK_ACK_BIT	0x80		// PS/2 clock interrupt acknowledge bit
#endif

#if (CHIP==M68000)
#define TIMER_FREQ	2457600L	// timer 3 input clock frequency
#endif

// clock task variables
PRIVATE clock_t realtime;		// real time clock
PRIVATE time_t	boot_time;		// time in seconds of system boot
PRIVATE clock_t next_alarm;		// possible time for the next alarm
PRIVATE message mc;			// both I/O message buffers
PRIVATE int watchdog_proc;		// when call *watch_dog[],consist proc_nr
PRIVATE watchdog_t watch_dog[NR_TASKS+NR_PROCS];

// variables used by both clock and synchronous alarm tasks
PRIVATE int syn_al_alive = TRUE;	// do not start syn_alrm_task before initial setup
PRIVATE int syn_table[NR_TASKS+NR_PROCS];	// which task gets CLOCK_INT

// variables modified by interrupt handlers
PRIVATE clock_t pending_ticks;		// clocks that only appear at low level
PRIVATE int sched_ticks = SCHED_RATE;	// count : when zero,call scheduler
PRIVATE struct proc *prev_ptr;		// last user process executed by
					// the clock task

FORWARD _PROTOTYPE(void common_setalarm,(int proc_nr,long delta_ticks,
			watchdog_t function));
FORWARD _PROTOTYPE(void do_clocktick,(void));
FORWARD _PROTOTYPE(void do_get_time,(void));
FORWARD _PROTOTYPE(void do_getuptime,(void));
FORWARD _PROTOTYPE(void do_set_time,(message *m_ptr));
FORWARD _PROTOTYPE(void do_setalarm,(message *m_ptr));
FORWARD _PROTOTYPE(void init_clock,(void));
FORWARD _PROTOTYPE(void cause_alarm,(void));
FORWARD _PROTOTYPE(void do_setsyn_alrm,(message *m_ptr));
FORWARD _PROTOTYPE(int clock_handler,(int irq));

/*======================================================================*
 * 				clock_task				*
 *======================================================================*/
PUBLIC void clock_task()
{
	/* main program of clock task. it corrects realtime by adding
	 * pending ticks seen only by the interrupt service,then it
	 * determines which of the 6 possible calls this is by looking
	 * at 'mc.m_type'. then it dispatches.
	 */

	int opcode;
	
	init_clock();			// initialize clock task

	// main loop of clock task.get work,process it,sometimes reply.
	while(TRUE){
		receive(ANY,&mc);	// get message
		opcode = mc.m_type;	// extract the function code

		lock();
		realtime += pending_ticks;	// transfer ticks from low level handler
		pending_ticks = 0;	// so we don't have to worry about them
		unlock();

		switch(opcode){
			case HARD_INT: do_clocktick();		break;
			case GET_UPTIME: do_getuptime();	break;
			case GET_TIME: do_get_time();		break;
			case SET_TIME: do_set_time(&mc);	break;
			case SET_ALARM: do_setalarm(&mc);	break;
			case SET_SYNC_AL: do_setsyn_alrm(&mc);	break;
			default: panic("clock task got bad message",mc.m_type);
		}

		// send reply,except for clock tick
		mc.m_type = OK;
		if (opcode != HARD_INT) send(mc.m_source,&mc);
	}
}

/*======================================================================*
 * 				do_clocktick				*
 *======================================================================*/
PRIVATE void do_cocktick()
{
	/* main program of clock task.it corrects realtime by adding pending
	 * ticks seen only by the interrupt service,then it determines
	 * which of the 6 possible calls this is by looking at 'mc.m_type'.
	 * then it dispatches.
	 */
	
	register struct proc *rp;
	register int proc_nr;

	if (next_alarm <= realtime){
		// even if the alarm goes off,the process may tstill be
		// there,so check it
		next_alarm = LONG_MAX;	// start calculating the next time
		for (rp=BEG_PROC_ADDR;rp<END_PROC_ADDR;rp++){
			if (rp->p_alarm != 0){
				// find out if the time for this alarm
				// has been reached
				if (rp->p_alarm <= realtime){
					/* the time has passed.if it is a user
					 * process,it signals it.if it is a 
					 * task,call a function pre-specified
					 * by the task
					 */
					proc_nr = proc_number(rp);
					if (watch_dog[proc_nr+NR_TASKS]){
						watchdog_proc = proc_nr;
						(*watch_dog[proc_nr+NR_TASKS])();
					}
					else
						cause_sig(proc_nr,SIGALRM);
					rp->p_alarm = 0;
				}

				// check which alarm will be next
				if (rp->p_alarm != 0 && rp->p_alarm < next_alarm)
					next_alarm = rp->p_alarm;
			}
		}
	}
	// if user process runs too long,select another process
	if (--sched_ticks == 0){
		if (bill_ptr == prev_ptr) lock_sched(); // the process has been
							// running for too long
		sched_ticks = SCHED_RATE;	// reset quantum
		prev_ptr = bill_ptr;		// immediate process
	}
}
			
			
/*======================================================================*
 * 				do_getuptime				*
 *======================================================================*/
PRIVATE void do_getuptime()
{
	/* get the current uptime by the numver of clocks and return */
	mc.NEW_TIME = realtime;		// current uptime
}


/*======================================================================*
 * 				get_uptime				*
 *======================================================================*/
PUBLIC clock_t get_uptime()
{
	/* get and return the current clock uptime in ticks.
	 * this function is designed to be called from other tasks,
	 * so they can get uptime without the overhead of message.
	 * it has to be careful about pending_ticks.
	 */
	clock_t uptime;

	lock();
	uptime = realtime + pending_ticks;
	unlock();
	return (uptime);
}


/*======================================================================*
 * 				do_get_time				*
 *======================================================================*/
PRIVATE void do_get_time()
{
	/* get the current clock time in seconds and return */
	mc.NEW_TIME = boot_time + realtime/HZ;		// current time
}

/*======================================================================*
 * 				do_set_time				*
 *======================================================================*/
PRIVATE void do_set_time(m_ptr)
message *m_ptr;				// pointer to required message
{
	/* set the real time clock. only the superuser can use this call */
	boot_time = m_ptr->NEW_TIME - realtime/HZ;
}


/*======================================================================*
 * 				do_setalarm				*
 *======================================================================*/
PRIVATE void do_setalarm(m_ptr)
message *m_ptr;			// pointer to required message
{
	/* a process needs an alarm signal,or the task needs a given
	 * watch_dog function that is called after a specified interval.
	 */
	register struct proc *rp;
	int proc_nr;			// which process needs an alarm
	long delta_ticks;		// what is the number of alarm clocks?
	watchdog_t function;		// function to call (only task)

	// extract parameter from message
	proc_nr = m_ptr->CLOCK_PROC_NR;		// process to interrupt after
	delta_ticks = m_ptr->DELTA_TICKS;	// clock number to wait
	function = (watchdog_t) m_ptr->FUNC_TO_CALL;	// function to call
							// (only task)
	rp = proc_addr(proc_nr);
	mc.SECONDS_LEFT = (rp->p_alarm == 0 ? 0 : (rp->p_alarm - realtime)/HZ);
	if (!istaskp(rp)) function = 0;		// user process receives signal
	common_setalarm(proc_nr,delta_ticks,function);
}

/*======================================================================*
 * 				do_setsyn_alrm				*
 *======================================================================*/
PRIVATE void do_setsyn_alrm(m_ptr)
message *m_ptr;			// pointer to required message
{
	/* process needs sync alarm */
	register struct proc *rp;
	int proc_nr;			// which process needs an alarm
	long delta_ticks;		// what is the number of alarm clocks?

	// extract parameters from message
	proc_nr = m_ptr->CLOCK_PROC_NR;	// the process of interrupting later
	delta_ticks = m_ptr->DELTA_TICKS;	// clock number to wait
	rp = proc_addr(proc_nr);
	mc.SECONDS_LEFT = (rp->p_alarm == 0 ? 0 : (rp->p_alarm - realtime)/HZ);
	common_setalarm(proc_nr,delta_ticks,cause_alarm);
}

/*======================================================================*
 * 				common_setalarm				*
 *======================================================================*/
PRIVATE void common_setalarm(proc_nr,delta_ticks,function)
int proc_nr;				// which process needs an alarm
long delta_ticks;			// how many clock number of alarm
watchdog_t function;			// function to call ( if cause_sig
					// is called,zero)
{
	/* complete the work of do_set_alarm and do_setsys_alarm.
	 * record an alarm request and check if it is the next
	 * required alarm.
	 */
	register struct proc *rp;
	
	rp = proc_addr(proc_nr);
	rp->p_alarm = (delta_ticks == 0 ? 0 : realtime + delta_ticks);
	watch_dop[proc_nr+NR_TASKS] = function;
	// which alarm is next?
	next_alarm = LONG_MAX;
	for (rp=BEG_PROC_ADDR;rp<END_PROC_ADDR;rp++)
		if (rp->p_alarm!=0 && rp->p_alarm < next_alarm)
			next_alarm = rp->p_alarm;
}

/*======================================================================*
 * 				cause_alarm				*
 *======================================================================*/
PRIVATE void cause_alarm()
{
	/* routine that is called when the timer expires and the process
	 * requests a synchronization alarm. the process number is in 
	 * the global variable watchdog_proc(HACK).
	 */
	message mess;

	syn_table[watchdog_proc + NR_TASKS] = TRUE;
	if (!syn_al_alive) send( SUN_ALRM_TASK,&mess);
}

/*======================================================================*
 * 				syn_alrm_task				*
 *======================================================================*/
PUBLIC void syn_alrm_task()
{
	/* main program of syn alarm task
	 * this task receives messages only from clock task cause_alarm.
	 * send a CLOCK_INT message to the process that required syn_alrm.
	 * a sync alarm is called a sync alarm by a process when it is 
	 * in a known part of the code,that is ,when it issues a call
	 * to receive a message,unlike a signal or watchdog activation.
	 */

	message mess;
	int word_done;		// is sleep executable?
	int *al_ptr;		// pointer of sync_table
	int i;

	syn_al_alive = TRUE;
	for (i=0,al_ptr=syn_table;i<NR_TASKS+NR_PROCS;i++,al_ptr++)
		*al_ptr = FALSE;

	while(TRUE){
		work_done = TRUE;
		for (i=0,al_ptr=syn_table;i<NR_TASKS+NR_PROCS;i++,al_ptr++)
			if (*al_ptr){
				*al_ptr = FALSE;
				mess.m_type = CLOCK_INT;
				send(i-NR_TASKS,&mess);
				work_done = FALSE;
			}
		if (work_done){
			syn_al_alive = FALSE;
			receive(CLOCK,&mess);
			syn_al_alive = TRUE;
		}
	}
}

/*======================================================================*
 * 				clock_handler				*
 *======================================================================*/
PRIVATE int clock_handler(irq)
int irq;
{
	register struct proc *rp;
	register unsigned ticks;
	clock_t now;

	if(ps_mca){
		// Ackowledge the PS/2 clock interrupt
		out_byte(PORT_B,in_byte(PORT_B) | CLOCK_ACK_BIT);
	}

	// update user and system accounting times.
	// first charge the current process for user time.
	// if the current process is not the billable process
	// (usually because it is a task),charge the billable process for sytem time
	// as well.thus the unbillable tasks' user time is the billable users'
	// system time.
	if (k_reenter != 0)
		rp = proc_addr(HARDWARE);
	else
		rp = proc_ptr;
	ticks = lost_ticks + 1;
	lost_ticks = 0;
	rp->user_time += ticks;
	if (rp != bill_ptr && rp != proc_addr(IDLE)) bill_ptr->sys_time += ticks;

	pending_ticks += ticks;
	now = realtime + pending_ticks;
	
	if (tty_timeout <= now) tty_wakeup(now);	// possibly wake up TTY
	pr_restart();					// possibly restart printer

	if (next_alarm <= now || sched_ticks == 1 && bill_ptr == prev_ptr &&
			rdy_head[USER_Q] != NIL_PROC){
		interrupt(CLOCK);
		return 1;		// reenable interrupt
	}

	if (--sched_ticks == 0){
		// if bill_ptr == prev_ptr,no ready users so don't need sched()
		sched_ticks = SCHED_RATE;		// reset quantum
		prev_ptr = bill_ptr;			// new previous process
	}
	return 1;			// reenable clock interrupt
}

/*======================================================================*
 * 				init_clock				*
 *======================================================================*/
PRIVATE void init_clock()
{
	// initialize channel 0 of the 8253A timer to e.g. 60HZ
	out_byte(TIMER_MODE,SQUARE_WAVE);	// set timer to run continuously
	out_byte(TIMER0,TIMER_COUNT);		// set low byte of timer
	out_byte(TIMER0,TIMER_COUNT >> 8);	// set hight byte of timer
	put_irq_handler(CLOCK_IRQ,clock_handler);	// set interrupt handler
	enable_irq(CLOCK_IRQ);			// ready for clock interrupt
}


/*======================================================================*
 * 				clock_stop				*
 *======================================================================*/
PUBLIC void clock_stop()
{
	// reset the clock to the BIOS rate.(for rebooting)
	out_byte(TIMER_MODE,0x36);
	out_byte(TIMER0,0);
	out_byte(TIMER0,0);
}

/*======================================================================*
 * 				milli_delay				*
 *======================================================================*/
PUBLIC void milli_delay(millisec)
unsigned millisec;
{
	// delay some milliseconds
	struct milli_state ms;
	milli_start(&ms);
	while(milli_elapsed(&ms) < millisec) {}
}

/*======================================================================*
 * 				milli_start				*
 *======================================================================*/
PUBLIC void milli_start(msp)
struct milli_state *msp;
{
	// prepare for calls to milli_elapsed()
	msp->prev_count = 0;
	msp->accum_count = 0;
}

/*======================================================================*
 * 				milli_elapsed				*
 *======================================================================*/
PUBLIC unsigned milli_elapsed(msp)
struct milli_state *msp;
{
	// return the number of milli seconds since the call to micro_start().
	// must be polled rapidly.
	unsigned count;

	out_byte(TIMER_MODE,LATCH_COUNT); // latch the chip copy count
	count = in_byte(TIMER0);	  // countdown continues for 2 steps
	count |= in_byte(TIMER0) << 8;

	msp->accum_count += count <= msp->prev_count ? (msp->prev_count - count) : 1;
	msp->prev_count = count;
	return msp->accum_count / (TIMER_FREQ/1000);
}
