// "table.c"'s object file contains all the data.in the *.h file,
// the declared variables are shown and EXTERN is added as follows:
//
// 	EXTERN int x;
//
// Normally,EXTERN is degined as extern,so if it is included in another file,
// it will not be assigned to the starage line. if EXTERN is an empty string and
// is simply indicated as
//
// 	int x;
//
// then 'x' will be declared several times if the source file contains a file with
// such a definition.
// but somewhere it must be declared.
//
// now redefine EXTERN as a null string and include add *.h files in table.c to 
// actually generate storage for them.
// since
//
// extern int x = 4;
//
// is not allowed, all initialized variables are also declare here.
// if such variables are shared, they must be declared in one of the *.h files without
// being initialized.
//
//
#define _TABLE

#include "kernel.h"
#include <termios.h>
#include <minix/com.h>
#include "proc.h"
#include "tty.h"

// for the startup routine of each task,the order is shown upward from -NR_TASKS.
// the order of the names here must match the number assigned to the task at <minix/com.h>.
#define SMALL_STACK		(128 * sizeof(char *))
#define TTY_STACK		(3 * SMALL_STACK)
#define SYN_ALRM_STACK		SMALL_STACK

#define DP8390_STACK		(SMALL_STACK * ENABLE_NERWORKING)

#if (CHIP == INTEL)
#define IDLE_STACK		((3+3+4) * sizeof(char *)) // 3 intr, 3 temps,4 db
#else
#define IDLE_STACK		SMALL_STACK
#endif // (CHIP == INTEL)

#define PRINTER_STACK		SMALL_STACK

#if (CHIP == INTEL)
#define WINCH_STACK		(2 * SMALL_STACK * ENABLE_WINI)
#else
#define WINCH_STACK		(3 * SMALL_STACK * ENABLE_WINI)
#endif

#if (MACHINE == ATARI)
#define SCSI_STACK		(3 * SMALL_STACK)
#endif // (MACHINE == ATARI)

#if (MACHINE == IBM_PC)
#define SCSI_STACK		(2 * SMALL_STACK * ENABLE_SCSI)
#endif // (MACHINE == IBM_PC)

#define CDROM_STACK		(4 * SMALL_STACK * ENABLE_CDROM)
#define AUDIO_STACK		(4 * SMALL_STACK * ENABLE_AUDIO)
#define MIXER_STACK		(4 * SMALL_STACK * ENABLE_AUDIO)

#define FLOP_STACK		(3 * SMALL_STACK)
#define MEM_STACK		SMALL_STACK
#define CLOCK_STACK		SMALL_STACK
#define SYS_STACK		SMALL_STACK
#define HARDWARE_STACK		0			// use dummy task,kernel stack

#define TOT_STACK_SPACE		(TTY_STACK+DP8390_STACK+SCSI_STACK+SYN_ALRM_STACK+IDLE_STACK+HARDWARE_STACK+PRINTER_STACK+WINCH_STACK+FLOP_STACK+MEM_STACK+CLOCK_STACK+SYS_STACK+CDROM_STACK+AUDIO_STACK+MIXER_STACK)

// SCSI,CDROM and AUDIO may make different choices like WINCHESTER in the future,
// but for the time being this choice will be fixed.
#define scsi_task		aha_scsi_task
#define cdrom_task		mcd_task
#define audio_task		dsp_task

// notes on the table below:
// 1) the tty_task must always be first so that printf can be used 
//    if other tasks have problems initializing
// 2) if you add a new kernel task,add it before the printer task
// 3) the task name is used for the process name(p_name)

PUBLIC	struct tasktab taskta[] = {
	{	tty_task,		TTY_STACK,		"TTY"	},
#if ENABLE_NETWORKING
	{	dp8390_task,		DP8390_STACK,		"DP8390"	},
#endif // ENABLE_NETWORKING
#if ENABLE_CDROM
	{	cdrom_task,		CDROM_STACK,		"CDROM"	},
#endif // ENABLE_CDROM
#if ENBALE_AUDIO
	{	audio_task,		AUDIO_STACK,		"AUDIO"	},
	{	mixer_task,		MIXER_STACK,		"MIXER"	},
#endif // ENABLE_AUDIO
#if ENABLE_SCSI
	{	scsi_task,		SCSI_STACK,		"SCSI"	},
#endif // ENABLE_SCSI
#if ENABLE_WINI
	{	winchester_task,	WINCH_STACK,		"WINCH"	},
#endif // ENABLE_WINI
	{	syn_alrm_task,		SYN_ALRM_STACK,		"SYN_AL"	},
	{	idle_task,		IDLE_STACK,		"IDLE"		},
	{	printer_task,		PRINTER_STACK,		"PRINTER"	},
	{	floppy_task,		FLOP_STACK,		"FLOPPY"	},
	{	mem_task,		MEM_STACK,		"MEMORY"	},
	{	clock_task,		CLOCK_STACK,		"CLOCK"		},
	{	sys_task,		SYS_STACK,		"SYS"		},
	{	0,			HARDWARE_STACK,		"HARDWARE"	},
	{	0,			0,			"MM"		},
	{	0,			0,			"FS"		},
#if ENABLE_NETWORKING
	{ 	0,			0,			"INET"		},
#endif // ENABLE_NETWORKING
	{	0,			0,			"INIT"		},
};	// PUBLIC struct tasktab taskta[]

// stack space for all task stacks ( declared as (char*) to adjust )
PUBLIC char *t_stack[TOT_STACK_SPACE / sizeof(char *)];

// the number of kernel tasks must be the same as NR_TASKS.
// Compile error if NR_TASKS is incorrect; array size is negative.
#define NKT (sizeof tasktab / sizeof(struct tasktab) - (INIT_PROC_NR + 1))

extern int dummy_tasktab_check[NR_TASKS == NKT ? 1 : -1];

