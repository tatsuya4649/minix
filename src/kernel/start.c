// this file contains the C startup code for minix on the Intel processor.
// work with mpx.s to set up an environment suitable for main().
//
// this code runs in real mode on a 16-bit kernel, but you may need to switch to
// protected mode on a 286.
//
// on a 32-bit kernel, this is already running in protected mode,
// but the selector was given by BIOS with interrupts still disabled,
// so reload the descriptor and create the interrupt descriptor. there is a need to.
//
//

#include "kernel.h"
#include <stdlib.h>
#include <minix/boot.h>
#include "protect.h"

PRIVATE char k_environ[256];			// environment string passed by the loader

FORWARD _PROTOTYPE( int k_atoi,(char *s));

/*======================================================================================*
 * 					cstart						*
 *======================================================================================*/

PUBLIC void cstart(cs,ds,mcs,mds,parmoff,parmsize)
U16_t cs,ds;					// kernel code and data segment
U16_t mcs,mds;					// monitor code and data segment
U16_t parmoff,parmsize;				// boot parameter offset and length
{
/* initalize the system before calling main() */

	register char *envp;
	phys_bytess mcode_base,mdata_base;
	unsigned mon_start;

	// record where the kernel and monitor are
	code_base = seg2phys(cs);
	data_base = seg2phys(ds);
	mcode_base = seg2phys(mcs);
	mdata_base = seg2phys(mds);

	// initialize protected mode descriptor
	prot_init();
	
	// copy boot parameter to kernel memory
	if (paramsize > sizeof k_environ - 2) paramsize = sizeof k_environ - 2;
	phys_copy(mdata_base + parmoff,vir2phys(k_environ),(phys_bytes) parmsize);

	// convert important boot environment variables
	boot_parameters.bp_rootdev = k_atoi(k_getenv("rootdev"));
	boot_parameters.bp_ramimagedev = k_atoi(k_getenv("ramimagedev"));
	boot_parameters.bp_ramsize = k_atoi(k_getenv("ramsize"));
	boot_parameters.bp_processor = k_atoi(k_getenv("processor"));

	// type of VDU
	envp = k_getenv("video");
	if (strcmp(envp,"ega") == 0) ega = TRUE;
	if (strcmp(envp,"vga") == 0) vga = ega = TRUE;

	// memory size
	low_memsize = k_atoi(k_getenv("memsize"));
	ext_memsize = k_atoi(k_getenv("emssize"));
	PUBLIC char *k_getenv(name)
	char *name;
	{
		// get the environment value.kernel version of getenv to avoid setting
		// the normal environment array
		register char *namep;
		register char *envp;

		for (envp = k_environ;*envp != 0;){
			for(namep = name; *namep != 0 && *namep == *envp;namep++,envp++)
				;
			if (*namep == '\0' && *envp == '=') return(envp + 1);
			while(*envp++ != 0)
				;
		}
		return (NIL_PTR);
	}

	// is processor?
	processor = boot_parameters.bp_processor;		// 86,186,286,386, ...

	// XT,AT or MCA bus?
	envp = k_getenv("bus");
	if (envp == NIL_PTR || strcmp(envp,"at") == 0){
		pc_at = TRUE;
	} else
	if (strcmp(envp,"mca") == 0){
		pc_at = ps_mcs = TRUE;
	}
	// determine if the mode if protected mode
#if _WORD_SIZE == 2
	protected_mode = processor >= 286;
#endif // _WORD_SIZE == 2

	// is there a monitor return to? keep it sage,if any
	if (!protected_mode) mon_return = 0;
	mon_start = mcode_base / 1024;
	if (mon_return && low_memsize > mon_start) low_memsize = mon_start;

	// return to assembler code,switch to protected mode,reload selector and call main().
	//
	//


/*======================================================================================*
 * 					k_atoi						*
 *======================================================================================*/
PRIVATE int k_atoi(s)
register char *s;
{
	// convert string to integer
	return strtol(s,(char **) NULL,10);
}
/*======================================================================================*
 * 					k_getenv					*
 *======================================================================================*/
PUBLIC char *k_getenv(name)
char *name;
{
	// get the environment value.kernel version of getenv to avoid setting
	// the normal environment array
	register char *namep;
	register char *envp;

	for (envp = k_environ;*envp != 0;){
		for(namep = name; *namep != 0 && *namep == *envp;namep++,envp++)
			PUBLIC char *k_getenv(name)
			char *name;
		{
			// get the environment value.kernel version of getenv to avoid setting
			// the normal environment array
			register char *namep;
			register char *envp;

			for (envp = k_environ;*envp != 0;){
				for(namep = name; *namep != 0 && *namep == *envp;namep++,envp++)
					;
				if (*namep == '\0' && *envp == '=') return(envp + 1);
				while(*envp++ != 0)
					;
			}
			return (NIL_PTR);
		}
			;
		if (*namep == '\0' && *envp == '=') return(envp + 1);
		while(*envp++ != 0)
			;
	}
	return (NIL_PTR);
}
