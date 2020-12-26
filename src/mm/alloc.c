

#include "mm.h"
#include <minix/com.h>

#define NR_HOLES	128
#define NIL_HOLE	(struct hole *) 0

PRIVATE struct hole{
	phys_clicks	h_base;
	phys_clicks	h_len;
	struct hole	*h_next;
} hole[NR_HOLES];

PRIVATE struct hole *hole_head;
PRIVATE struct hole *free_slots;

FORWARD _PROTOTYPE(void del_slot,(struct hole *prev_ptr,struct hole *hp));
FORWARD _PROTOTYPE(void merge,(struct hole *hp));

/*======================================================================*
 * 				alloc_mem				*
 *======================================================================*/
PUBLIC phys_clicks alloc_mem(clicks)
phys_clicks clicks;
{
	register struct hole *hp,*prev_ptr;
	phys_clicks old_base;
	
	hp = hole_head;
	while (hp != NIL_HOLE){
		if (hp->h_len >= clicks){
			old_base = hp->h_base;
			hp->h_base += clicks;
			hp->h_len -= clicks;
			
			if (hp->h_len != 0) return (old_base);
			
			del_slot(prev_ptr,hp);
			return (old_base);
		}
		prev_ptr = hp;
		hp = hp->h_next;
	}
	return (NO_MEM);
}

/*======================================================================*
 * 				free_mem				*
 *======================================================================*/
PUBLIC void free_mem(base,clicks)
phys_clicks base;
phys_clicks clicks;
{
	register struct hole *hp,*new_ptr,*prev_ptr;

	if (clicks == 0)return;
	if ((new_ptr=free_slots) == NIL_HOLE) panic("Hole table full",NO_NUM);
	new_ptr->h_base = base;
	new_ptr->h_len = clicks;
	free_slots = new_ptr->h_next;
	hp = hole_head;

	if (hp == NIL_HOLE || base <= hp->h_base){
		new_ptr->h_next = hp;
		hole_head = new_ptr;
		merge(new_ptr);
		return;
	}

	while(hp!=NIL_HOLE && base > hp->h_base){
		prev_ptr = hp;
		hp = hp->h_next;
	}

	new_ptr->h_next = prev_ptr->h_next;
	prev_ptr->h_next = new_ptr;
	merge(prev_ptr);
}

/*======================================================================*
 * 				del_slot				*
 *======================================================================*/
PRIVATE void del_slot(prev_ptr,hp)
register struct hole *prev_ptr;
register struct hole *hp;
{
	if (hp == hole_head)
		hole_head = hp->hp_next;
	else
		prev_ptr->h_next = hp->h_next;
	hp->h_next = free_slots;
	free_slots = hp;
}

/*======================================================================*
 * 				merge					*
 *======================================================================*/
PRIVATE void merge(hp)
register struct hole *hp;
{
	register struct hole *next_ptr;

	if ((next_ptr=hp.h_next) == NIL_HOLE) return;
	if (hp->h_base+hp->h_len == next_ptr->h_base){
		hp->h_len += next_ptr->h_len;
		del_slot(hp,next_ptr);
	}else{
		hp = next_ptr;
	}

	if ((next_ptr=hp->h_next) == NIL_HOLE) return;
	if (hp->h_base+hp->h_len == next_ptr->h_base){
		hp->h_len += next_ptr->h_len;
		del_slot(hp,next_ptr);
	}
}

/*======================================================================*
 * 				max_hole				*
 *======================================================================*/
PUBLIC phys_clicks max_hole()
{
	register struct hole *hp;
	register phys_clicks max;

	hp = hole_head;
	max = 0;
	while( hp!=NIL_HOLE ){
		if (hp->h_len > max) max = hp->h_len;
		hp = hp->h_next;
	}
	return (max);
}

/*======================================================================*
 * 				mem_init				*
 *======================================================================*/
PUBLIC void mem_init(total,free)
phys_clicks *total,*free;
{
	register struct hole *hp;
	phys_clicks base;
	phys_clicks size;
	message mess;

	for (hp=&hole[0];hp<&hole[NR_HOLES];hp++) hp->h_next = hp+1;
	hole[NR_HOLES-1].h_next = NIL_HOLE;
	hole_head = NIL_HOLE;
	free_slots = &hole[0];

	*free = 0;
	for (;;){
		mess.m_type = SYS_MEM;
		if (sendrec(SYSTASK,&mess) != OK) panic("bad SYS_MEM?",NO_NUM);
		base = mess.m1_i1;
		size = mess.m1_i2;
		if (size==0) break;

		free_mem(base,size);
		*total = mess.m1_i3;
		*free += size;
	}
}
