// this file contains the main MINIX program. the routine main() starts
// by initializing the system, setting the proc table,interrupt vector,
// and scheduling each task to run for initalizing.
//
// the entries in this file are:
// 	main:		minix main program
// 	panic:		minix stopped due to a fatal error
//

#include "kernel.h"
#include <signal.h>
#include <unistd.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "proc.h"

/*==============================================================================*
 * 					main					*
 *==============================================================================*/
PUBLIC	void main()
{
	/* start */
	register struct proc *rp;
	register int t;
	int sizeindex;
	phys_clicks text_base;
	vir_clicks text_clicks;
	vir_clicks data_clicks;
	phys_bytes phys_b;
	reg_t ktsb;				// kernel task stack base
	struct memory *memp;
	struct tasktab *ttp;

	// initialize interrupt controller
	intr_init(1);

	// interpret memory size
	mem_init();

	// clear process table
	// set mapping for proc_addr() and proc_number() macros
	for (rp = BEG_PROC_ADDR,t=-NR_TASKS;rp<END_PROC_ADDR;++rp,++t){
		rp->p_flas = P_SLOT_FREE;
		rp->p_nr = t;				// process number from pointer
		(pproc_addr + NR_TASKS)[t] = rp;	// process pointer from number
	}

	// set process table entries for tasks and servers. the kernle task stack is
	// initalized as an array in the data space. the stack pointer is set at the 
	// end of the data segment because the server stack has been added to the data
	// segment by the monitor.
	// all processes are in 8086 lower memory.in 386,only the kernel is in lower
	// memory and the rest is loaded in extended memory.
	
	// task stack
	ktsb = (reg_t) t_stack;

	for (t = -NR_TASKS; t<=LOW_USER;++t){
		rp = proc_addr(t);			// process slot of t
		ttp = &tasktab[t + NR_TASKS];		// task attribute of t
		strcpy(rp->p_name,ttp->name);
		if (t<0){
			if (ttp->stksize > 0){
				rp->p_stguard = (reg_t *) ktsb;
				*rp->p_stguard = STACK_GUARD;
			}
			ktsb += ttp->stksize;
			rp->p_reg.sp = ktsb;
			text_base = code_base >> CLICK_SHIFT; 	// all tasks are in the kernel and
							      	// use the entire kernel
			sizeindex = 0;
			memp = &mem[0];				// remove from this chunk of memory
		}else{
			sizeindex = 2 * t + 2;			// MM,FS,INIT has unique size
		}
		rp->p_reg.pc = (reg_t) ttp->initial_pc;
		rp->p_reg.psw = istaskp(rp) ? INIT_TASK_PSW : INIT_PSW;

		text_clicks = sizes[sizeindex];
		data_clicks = sizes[sizeindex + 1];
		rp->p_map[T].mem_phys = text_base;
		rp->p_map[T].mem_len = text_clicks;
		rp->p_map[D].mem_phys = text_base + text_clicks;
		rp->p_map[D].mem_len = data_clicks;
		rp->p_map[S].mem_phys = text_base + text_clicks + data_clicks;
		rp->p_map[S].mem_vir = data_clicks;		// free -- stack is in data
		text_base += text_clicks + data_clicks;		// for servers,prepare for
		memp->size -= (text_base - memp->base);
		memp->base = text_base;				// memory is no longer free
		
		if(t >= 0){
			// initialize the server pointer. lower the server stack pointer
			// by one word to give crtso.s "args"
			rp->p_reg.sp = (rp->p_map[S].mem_vir + rp->p_map[S].mem_len) << CLICK_SHIFT;
			rp->p_reg.sp -= sizeof(reg_t);
		}
#if _WORD_SIZE == 4
		// in 386mode, the server is loaded into external memory.
		if(t<0){
			memp = &mem[1];
			text_base = 0x100000 >> CLICK_SHIFT;
		}
#endif	// _WORD_SIZE == 4
		if(!isidlehardware(t)) lock_ready(rp);		// is the hardware waiting?
		rp->p_flags = 0;
		alloc_segments(rp);
	}

	proc[NR_TASKS+INIT_PROC_NR].p_pid = 1;			// course INIT is pid 1
	bill_ptr = proc_addr(IDLE);				// must point somewhere
	lock_pick_proc();
	
	// now call the assembly code to start the execution of the current process
	restart();
}	

/*==============================================================================*
 * 					panic					*
 *==============================================================================*/
PUBLIC void panic(s,n)
_CONST char *s;
int n;
{
	// the system suspends with a fatal error and terminates execution.
	// if the MM or FS panics,the string will be empty and the file system
	// has already been checked for integrity. when the kernel panics,
	// it becomes a kind of unresponsive state.
	if (*s != 0){
		printf("\nKernel panic: %s",s);
		if (n != NO_NUM) printf(" %d",n);
		printf("\n");
	}
	wreboot(RBT_PANIC);
}	
