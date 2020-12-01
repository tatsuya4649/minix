#include "mm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "mproc.h"
#include "param.h"

FORWARD _PROTOTYPE(void get_work,(void));
FORWARD _PROTOTYPE(void mm_init,(void));

/*======================================================================*
 * 				main					*
 *======================================================================*/
PUBLIC void main()
{
	int error;

	mm_init();

	while(TRUE){
		get_work();
		mp = &mproc[who];

		error = OK;
		dont_reply = FALSE;
		err_code = -999;

		if (mm_call < 0 || mm_call >= NCALLS)
			error = EBADCALL;
		else
			error = (*call_vec[mm_call])();

		if (dont_reply) continue;
		if (mm_call == EXEC && error == OK) continue;
		reply(who,error,result2,res_ptr);
	}
}

/*======================================================================*
 * 				get_work				*
 *======================================================================*/
PRIVATE void get_work()
{
	if (receive(ANY,&mm_in) != OK) panic("MM receive error",NO_NUM);
	who = mm_in.m_source;
	mm_call = mm_in.m_type;
}

/*======================================================================*
 * 				reply					*
 *======================================================================*/
PUBLIC void reply(proc_nr,result,res2,respt)
int proc_nr;
int result;
int res2;
char *respt;
{
	register struct mproc *proc_ptr;

	proc_ptr = &mproc[proc_nr];
	
	if ((who>=0) && ((proc_ptr->mp_flags&IN_USE) == 0 || (proc_ptr->mp_flags&HANGING))) return;
	reply_type = result;
	reply_i1 = res2;
	reply_p1 = respt;
	if (send(proc_nr,&mm_out) != OK) panic("MM can't reply",NO_NUM);
}

/*======================================================================*
 * 				mm_init					*
 *======================================================================*/
PRIVATE void mm_init()
{
	static char core_sigs[] = {
		SIGQUIT,SIGILL,SIGTRAP,SIGABRT,
		SIGEMT,SIGFPE,SIGUSR1,SIGSEGV,
		SIGUSR2,0};
	register int proc_nr;
	register struct mproc *rmp;
	register char *sig_ptr;
	phys_clicks ram_clicks,total_clicks,minix_clicks,free_clicks,dummy;
	message mess;
	struct mem_map kernel_map[NR_SEGS];
	int mem;

	sigemptyset(&core_sset);
	for (sig_ptr=core_sigs;*sig_ptr!=0;sig_ptr++)
		sigaddset(&core_sset,*sig_ptr);
	
	sys_getmap(SYSTASK,kernel_map);
	minix_clicks = kernel_map[S].mem_phys + kernel_map[S].mem_len;

	for(proc_nr=0;proc_nr<=INIT_PROC_NR;proc_nr++){
		rmp = &mproc[proc_nr];
		rmp->mp_flags |= IN_USE;
		sys_getmap(proc_nr,rmp->mp_seg);
		if (rmp->mp_seg[T].mem_len != 0) rmp->mp_flags |= SEPARATE;
		minix_clicks += (rmp->mp_seg[S].mem_phys + rmp->mp_seg[S].mem_len)
			- rmp->mp_seg[T].mem_phys;
	}
	mproc[INIT_PROC_NR].mp_pid = INIT_PID;
	sigemptyset(&mproc[INIT_PROC_NR].mp_ignore);
	sigemptyset(&mproc[INIT_PROC_NR].mp_catch);
	procs_in_use = LOW_USER + 1;

	if (receive(FS_PROC_NR,&mess) != OK)
		panic("MM can't obtain RAM disk size from FS",NO_NUM);
	ram_clicks = mess.m1_i1;

	mem_init(&total_clicks,&free_clicks);

	printf("\nMemory size =%5dK	",click_to_round_k(total_clicks));
	printf("MINIX =%4dK	",click_to_round_k(minix_clicks));
	printf("RAM disk =%5dK	",click_to_round_k(ram_clicks));
	printf("Available =%5dK	",click_to_round_k(free_clicks));

	if (send(FS_PROC_NR,&mess) != OK)
		panic("MM can't sync up with FS",NO_NUM);

	if ((mem = open("/dev/mem",O_RDWR)) != -1){
		ioctl(mem,MIOCSPSINFO,(void *) mproc);
		close(mem);
	}
}
