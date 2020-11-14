// select a winchester driver
//
// you can compile several different winchester drivers inside the kernel,
// but you can only run one.
// one of its drivers is seleected here using boot variables 'hd'.
//

#include "kernel.h"
#include "driver.h"

#if ENABLE_WINI

// map the driver name to the task function
struct hdmap{
	char	*name;
	task_t	*task;
} hdmap[] = {

#if ENABLE_AT_WINI
	{"at",	at_winchester_task},
#endif

#if ENABLE_BIOS_WINI
	{"bios", bios_winchester_task},
#endif

#if ENABLE_ESDI_WINI
	{"esdi", esdi_winchester_task},
#endif

#if ENABLE_XT_WINI
	{"xt", xt_winchester_task},
#endif
};

/*======================================================================================*
 * 					winchester_task					*
 *======================================================================================*/
PUBLIC void winchester_task()
{
	// invokes the default or selected winchester task
	char *hd;
	struct hdmap *map;

	hd = k_getenv("hd");

	for (map=hdmap;map<hdmap+sizeof(hdmap)/sizeof(hdmap[0]);map++){
		if (hd==NULL || strcmp(hd,map->name) == 0){
			// perform the selected winchester task
			(*map->task)();
		}
	}
	panic("no hd driver",NO_NUM);
}
#endif // ENABLE_WINI
