
#include "mm.h"
#include <signal.h>
#include "mproc.h"
#include "param.h"

#define DATA_CHANGED		1
#define STACK_CHANGED		2

/*======================================================================*
 * 				do_brk					*
 *======================================================================*/
PUBLIC int do_brk()
{
	register struct mproc *rmp;
	int r;
	vir_bytes v,new_sp;
	vir_clicks new_clicks;

	rmp = mp;
	v = (vir_bytes) addr;
	new_clicks = (vir_clicks) ( ((long) v + CLICK_SIZE - 1) >> CLICK_SHIFT);
	if (new_clicks < rmp->mp_seg[D].mem_vir){
		res_ptr = (char *) -1;
		return (ENOMEM);
	}
	new_clicks -= rmp->mp_seg[D].mem_vir;
	sys_getsp(who,&new_sp);
	r = adjust(rmp,new_clicks,new_sp);
	res_ptr = (r == OK ? addr : (char *) -1);
	return (r);
}

/*======================================================================*
 * 				adjust					*
 *======================================================================*/
PUBLIC int adjust(rmp,data_clicks,sp)
register struct mproc *rmp;
vir_clicks data_clicks;
vir_bytes sp;
{
	register struct mem_map *mem_sp,*mem_dp;
	vir_clicks sp_click,gap_base,lower,old_clicks;
	int changed,r,ft;
	long base_of_stack,delta;

	mem_dp = &rmp->mp_seg[D];
	mem_sp = &rmp->mp_seg[S];
	changed = 0;

	if (mem_sp->mem_len == 0) return (OK);

	base_of_stack = (long) mem_sp->mem_vir + (long) mem_sp->mem_len;
	sp_click = sp >> CLICK_SHIFT;
	if (sp_click >= base_of_stack) return(ENOMEM);

	delta = (long) mem_sp->mem_vir - (long) sp_click;
	lower = (delta > 0 ? sp_click : mem_sp->mem_vir);

#define SAFETY_BYTES	(384 * sizeof(char *))
#define SAFETY_CLICKS	((SAFETY_BYTES + CLICK_SIZE - 1) / CLICK_SIZE)
	gep_base = mem_dp->mem_vir + data_clicks + SAFETY_CLICKS;
	if (lower < gap_base) return (ENOMEM);

	old_clicks = mem_dp->mem_len;
	if (data_clicks != mem_dp->mem_len){
		mem_dp->mem_len = data_clicks;
		changed |= DATA_CHANGED;
	}

	if (delta > 0){
		mem_sp->mem_vir -= delta;
		mem_sp->mem_phys -= delta;
		mem_sp->mem_len += delta;
		changed |= STACK_CHANGED;
	}

	ft = (rmp->mp_flags & SEPARATE);
	r = size_ok(ft,rmp->mp_seg[T].mem_len,rmp->mp_seg[D].mem_len,
			rmp->mp_seg[S].mem_len,rmp->mp_seg[D].mem_vir,rmp->mp_seg[S].mem_vir);
	if (r == OK){
		if (changed) sys_newmap((int) (rmp - mproc),rmp->mp_seg);
		return (OK);
	}

	if (changed & DATA_CHANGED) mem_dp->mem_len = old_clicks;
	if (changed & STACK_CHANGED){
		mem_sp->mem_vir += delta;
		mem_sp->mem_phys += delta;
		mem_sp->mem_len += delta;
	}
	return (ENOMEM);
}

/*======================================================================*
 * 				size_ok					*
 *======================================================================*/
PUBLIC int size_ok(file_type,tc,dc,sc,dvir,s_vir)
int file_type;
vir_clicks tc;
vir_clicks dc;
vir_clicks sc;
vir_clicks dvir;
vir_clicks s_vir;
{
#if (CHIP == INTEL && _WORD_SIZE == 2)
	int pt,pd,ps;

	pt = ((tc<<CLICK_SHIFT) + PAGE_SIZE - 1)/PAGE_SIZE;
	pd = ((dc<<CLICK_SHIFT) + PAGE_SIZE - 1)/PAGE_SIZE;
	ps = ((sc<<CLICK_SHIFT) + PAGE_SIZE - 1)/PAGE_SIZE;

	if (file_type==SEPARATE){
		if (pt>MAX_PAGES || pd + ps > MAX_PAGES) return (ENOMEM);
	}else{
		if (pt+pd+ps > MAX_PAGES) return (ENOMEM);
	}
#endif
	if (dvir + dc > s_vir) return (ENOMEM);
	return (OK);
}
