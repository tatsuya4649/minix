// this file contains a collection of miscellaneous procedures:
// 	*	mem_init: 	initialize memory tables.
// 	*	env_parse	parse environment variables
// 	*	bad_assertion	for debugging
// 	*	bad_compare	for debugging

#include "kernel.h"
#include "assert.h"
#include <stdlib.h>
#include <minix/com.h>

#define EM_BASE		0x100000L	// AT extended memory base
#define SHADOW_BASE	0xFA0000L	// RAM base to shadow AT ROM
#define SHADOW_MAX	0x060000L	// maximum usable shadow memory (16M limit)

/*======================================================================================*
 * 					mem_init					*
 *======================================================================================*/
PUBLIC void mem_init()
{
	// initialize the memory size table.this is complicated by fragmentation and
	// various access strategies for protected modes. at 0,there must be a mass large
	// enough to hold the minix itself.processors 286 and 386 may have extended memory
	// (1MB or more of memory).this usually starts at 1MB,but there is another chunk
	// under 16MB that is also available for minix if the hardware can be remapped
	// while being reserved under DOS to shadow the ROM.there may be.
	// In protected mdoe,extended memory can be accessed on the assumption that CLICK_SIZE
	// is large enough to be treated as normal memory.
	
	u32_t		ext_clicks;
	phys_clicks	max_clicks;

	// get normal memory size from BIOS
	mem[0].size = k_to_click(low_memsize);		// base = 0
	if (pc_at && protected_mode){
		// get the size of extended memory from the BIOS. this is special
		// except in protected mode,but at this stage protected mode is normal.
		// in 286 mode,you cannot specify an address of 16M or more,so make sure
		// that the click count of the maximum memory address is shor.
		ext_clicks = k_to_click((u32_t) ext_memsize);
		max_clicks = USHRT_MAX - (EM_BASE >> CLICK_SHIFT);
		mem[1].size = MIN(ext_clicks,max_clicks);
		mem[1].base = EM_BASE >> CLICK_SHIFT;

		if (ext_memsize <= (unsigned) ((SHADOW_BASE - EM_BASE) / 1024) && check_mem(SHADOW_BASE,SHADOW_MAX) == SHADOW_MAX){
			// shadow ROM memory
			mem[2].size = SHADOW_MAX >> CLICK_SHIFT;
			mem[2].base = SHADOW_BASE >> CLICK_SHIFT;
		}
	}

	// system memory statistics
	tot_mem_size = mem[0].size + mem[1].size + mem[2].size;
}
	
		
/*======================================================================================*
 * 					env_parse					*
 *======================================================================================*/
PUBLIC int env_parse(env,fmt,field,param,min,max)
char *env;				// environment variable to inspect
char *fmt;				// template to parse it with
int field;				// field number of value to return
long *param;				// address of parameter to get
long min,max;				// minumum and maximum values for the parameter
{
	// parse the environment variable setting,something like DPETHO=300:3.
	// panic if the parsing fails. return EP_UNSET if the environment variable is not
	// set, EP_OFF if it is set to "off",EP_ON if set to "on" or a field is left blank,
	// or EP_SET if a field is given (return value through *param). punctuation may be
	// used in the environment and format string,fields in the environment string may be
	// empty,and punction may be missing to skip fields. the format string contains
	// characters 'd','o','x' and 'c' to indicate that 10,8,16 or 0 is used as the last
	// argument to strtol(). a '*' means that a field should be skipped. if the format
	// string contains something like "\4" then the string is repeated 4 characters to the left
	
	char *val,*end;
	long newpar;
	int i = 0,radix,r;

	if ((val = k_getenv(env)) == NIL_PTR) return (EP_UNSET);
	if (strcmp(val,"off") == 0) return (EP_OFF);
	if (strcmp(val,"on") == 0) return (EP_ON);

	r = EP_ON;
	for (;;){
		while(*val == ' ') val++;
		
		if (*val == 0) return (r);		// the proper exit point
		
		if (*fmt == 0) break;			// too many values
	
		if (*val == ',' || *val == ':'){
			// go to next field
			if (*fmt == ',' || *fmt == ':') i++;
			if (*fmt++ == *val) val++;
		}else{
			// get environment variables
			switch(*fmt){
				case 'd':	radix =   10;	break;
				case 'o':	radix =  010;	break;
				case 'x':	radix = 0x10;	break;
				case 'c':	radix =	   0;	break;
				default:	goto badenv;
			}
			newpar = strtol(val,&end,radix);

			if (end == val) break;	// not number
			val = end;

			if (i == field){
				// required field
				if (newpar < min || newpar > max) break;
				*param = newpar;
				r = EP_SET;
			}
		}
	}
badenv:
	printf("Bad environment setting: '%s = %s'\n",env,k_getenv(env));
	panic("",NO_NUM);
	// NOTREACHED
}

#if DEBUG

/*======================================================================================*
 * 					bad_assertion					*
 *======================================================================================*/
PUBLIC void bad_assertion(file,line,what)
char *file;
int line;
char *what;
{
	print("panic at %s(%d) : assertion \"%s\" failed\n",file,line,what);
	panic(NULL,NO_NUM);
}
/*======================================================================================*
 * 					bad_compare					*
 *======================================================================================*/
PUBLIC void bad_compare(file,line,lhs,what,rhs)
char *file;
int line;
int lhs;
char *what;
int rhs;
{
	printf("panic at %s(%d) : compare (%d) %s (%d) failed\n",file,line,lhs,what,rhs);
	panic(NULL,NO_NUM);
}
#endif // DEBUG

