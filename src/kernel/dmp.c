
#include "kernel.h"
#include <minix/com.h>
#include "proc.h"

char *vargv;

FORWARD _PROTOTYPE(char *proc_name,(int proc_nr));
/*======================================================*
 * 			p_dmp				*
 *======================================================*/
PUBLIC void p_dmp()
{
	register struct proc *rp;
	static struct proc *oldrp = BEG_PROC_ADDR;
	int n = 0;
	phys_clicks text,data,size;
	int proc_nr;
	printf("\n--pid --pc- ---sp- flag -user --sys-- -text- -data- -size- -recv- command\n");
	
	for(rp=oldrp;rp<END_PROC_ADDR;rp++){
		proc_nr = proc_number(rp);
		if (rp->p_flags & P_SLOT_FREE) continue;
		if (++n > 20) break;
		text = rp->p_map[T].mem_phys;
		data = rp->p_map[D].mem_phys;
		size = rp->p_map[T].mem_len +
			 ((rp->p_map[S].mem_phys + rp->p_map[S].mem_len) - data);
		printf("%5d %5lx %6lx %2x %7U %7U %5uK %5uK %5uK ",
				proc_nr < 0 ? proc_nr : rp->p_pid,
				(unsigned long) rp->p_reg.pc,
				(unsigned long) rp->p_reg.sp,
				rp->p_flags,
				rp->user_time,rp->tsys_time,
				click_to_round_k(text),click_to_round_k(data),
				click_to_round_k(size));
		if (rp->p_flags & RECEIVING){
			printf("%-7.7s",proc_name(rp->p_getfrom));
		}else
		if (rp->p_flags & SENDING){
			printf("S:%-5.5s",proc_name(rp->p_sendto));
		}else
		if (rp->p_flags == 0){
			printf("	");
		}
		printf("%s\n",rp->p_name);
	}
	if (rp==END_PROC_ADDR) rp = BEG_PROC_ADDR; else printf("--more--\r");
	oldrp = rp;
}

/*======================================================*
 * 			map_dmp				*
 *======================================================*/
PUBLIC void map_dmp()
{
	register struct proc *rp;
	static struct proc *oldrp = cproc_addr(HARDWARE);
	int n = 0;
	phys_clicks size;

	printf("\nPROC NAME- -----TEXT----- -----DATA------ ----STACK---- -SIZE-\n");
	for(rp=oldrp;rp<END_PROC_ADDR;rp++){
		if (rp->p_flag & P_SLOT_FREE) continue;
		if (++n > 20) break;
		size = rp->p_map[T].mem_len
			+ ((rp->p_map[2].mem_phys + rp->p_map[S].mem_len) - rp->p_map[D].mem_phys);
		printf("%3d %-6.6s %4x %4x %4x  %4x %4x %4x  %4x %4x %4x  %5uK\n",
				proc_number(rp),
				rp->p_name,
				rp->p_map[T].mem_vir,rp->p_map[T].mem_phys,rp->p_map[T].mem_len,
				rp->p_map[D].mem_vir,rp->p_map[D].mem_phys,rp->p_map[D].mem_len,
				rp->p_map[S].mem_vir,rp->p_map[S].mem_phys,rp->p_map[S].mem_len,
				click_to_round_k(size));
	}
	if (rp==END_PROC_ADDR) rp = cproc_addr(HARDWARE); else printf("--more--\r");
	oldrp = rp;
}

/*======================================================*
 * 			proc_name			*
 *======================================================*/
PRIVATE char *proc_name(proc_nr)
int proc_nr;
{
	if (proc_nr == ANY) return "ANY";
	return proc_addr(proc_nr)->p_name;
}
