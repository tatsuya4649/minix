// global variables used in kernel
//
//

// EXTERN is defined as extern except table.c
#ifndef _TABLE
#undef EXTERN
#define EXTERN
#endif

// kernel memory
EXTERN phys_bytes code_base;		// base of kernel code
EXTERN phys_bytes data_base;		// base of kernel data

// low level interrupt communication
EXTERN struct proc *held_head;		// the beginning of the hold-up interrupt queue
EXTERN struct proc *held_tail;		// end of the hold-up interrupt queue
EXTERN unsigned char k_reenter;		// kernel reentry count ( entry count - 1 )

// process table. there are many other things to do to include proc.h
EXTERN struct proc *proc_ptr;		// a pointer to the currently running process

// signal
EXTERN int sig_procs;			// number of process of p_pending != 0

// memory size
EXTERN struct memory mem[NR_MEMS];	// base and size of memory
EXTERN phys_clicks tot_mem_size;	// total size of system memory

// other
extern u16_t size[];			// table filled by boot monitor
extern struct tasktab tasktab[];	// since it is initialized in table.c,extern here
extern char *t_stack[];			// since it is initialized in table.c,extern here
EXTERN unsigned lost_ticks;		// the number of clocks is counted outside the clock task
EXTERN clock_t tty_timeout;		// TTY task launch time
EXTERN int current;			// currently visiable console

#if (CHIP == INTEL)
// machine types
EXTERN int pc_at;			// PC-AT compatible hardware interface
EXTERN int ps_mca;			// PS/2 with micro channel
EXTERN unsigned int processor;		// 86,186,286,386,...
#if _WORD_SIZE == 2
EXTERN int protected_mode;		// non-zero when runnning in Intel protected mode
#else
#define protected_mode	1		// 386 mode means protected mode
#endif // (CHIP == INTEL)

// video card type
EXTERN int ega;				// non-zero when console is EGA or VGA
EXTERN int vga;				// non-zero when console is VGA

// memory size
EXTERN unsigned ext_memsize;		// initialize with assembler startup code
EXTERN unsigned low_memsize;

// other
EXTERN irq_handler_t irq_table[NR_IRQ_VECTORS];
EXTERN int irq_use;			// bitmap of all in-use IRQs
EXTERN reg_t mon_ss,mon_sp;		// monitor stack
EXTERN int mon_return;			// true if it is possible to return to the monitor
EXTERN phys_bytes reboot_code;		// program for boot monitor

// variables that are initialized externally are set to extern here.
extern struct segdesc_s gdt[];		// protected mode global description table

EXTERN _PROTOTYPE( void (*level0_func),(void));
#if (CHIP == M68000)
// variables that are initialized externally are set to extern here.
extern int keypad;			// flag for keypad mode
extern int app_mode;			// flag for arrow key application mode
extern int STdebKey;			// non-zero when ctl-alt-Fx is detected
extern struct tty *cur_cons;		// virtual console is showing now
extern unsigned char font8[];		// 8-pixel width font table (already initalize)
extern unsigned char font12[];		// 12-pixel width font table (already initalize)
extern unsigned char font16[];		// 16-pixel width font table (already initialize)
extern unsigned short resolution;	// image resolution; ST_RES_LOW..TT_RES_HIGH
#endif // (CHIP == M68000)





