// general constants used by the kernel
//
// 

#if (CHIP == INTEL)
#define K_STACK_BYTES		1024		// kernel stack bytes

#define INIT_PSW		0x0200		// initial psw
#define INIT_TASK_PSW		0x1200		// initial psw of task(IOPL is 1)
#define TRACEBIT		0x100		// take psw and OR of proc[] and use them for tracing
#define SETPSW(rp,new)	/* allow setting only certain bits */ \
	((rp)->p_reg.psw = (rp)->p_reg.psw & 0xCD5 | (new) & 0xCD5)

// initial sp for mm,fs and init
//	* 2bytes for short jamp
//	* 2bytes are not used
//	* 3words for init_org[] used by only fs
//	* real mode debugger trap is 3words( actually need one more )
//	* 3words for temporary save and restart
//	* 3words for interrupt
// no margin left to flush bugs early
//
#define INIT_SP	(2 + 2 + 3*2 + 3*2 + 3*2 + 3*2)

#define HCLICK_SHIFT		4		// log2 of HCLICK_SIZE
#define HCLICK_SIZE		16		// hardware segment conversion magic
#if CLICK_SIZE >= HCLICK_SIZE
#define click_to_hclick(n)	((n) << (CLICK_SHIFT - HCLICK_SHIFT))
#else
#define click_to_hclick(n)	((n) >> (HCLICK_SHIFT - CLICK_SHIFT))
#endif // CLICK_SIZE >= HCLICK_SIZE

#define hclick_to_physb(n)	((phys_bytes) (n) << HCLICK_SHIFT)
#define physb_to_hclick(n)	((n) >> HCLICK_SHIFT)

// interrupt vector defined/reserved by the processor
#define DIVIDE_VECTOR		0		// split error
#define DEBUG_VECTOR		1		// single step (trace)
#define NMI_VECTOR		2		// unmaskable interrupt
#define BREAKPOINT_VECTOR	3		// software break point
#define OVERFLOW_VECTOR		4		// from INTO

// fixed sysytem call vector
#define SYS_VECTOR		32		// make system calls with int SYSVEC
#define SYS386_VECTOR		33		// use this except 386 system call
#define LEVEL0_VECTOR		34		// for function execution in lebel 0

//  IRQ base suitable for hardware interrupts. the BIOS does not respect the vectors
//  reserverd for all processors and reprograms using the default settings in the PC BIOS.
#define BIOS_IRQ0_VEC		0x08		// IRQ0-7 vector base used by BIOS
#define BIOS_IRQ8_VEC		0x70		// IRQ8-15 vector base used by BIOS
#define IRQ0_VECTOR		0x28		// more or less optional,but greater than SYS_VECTOR
#define IRQ8_VECTOR		0x30		// bulk for brevity

// hardware interrupt number
#define NR_IRQ_VECTOR		16
#define CLOCK_IRQ		0
#define KEYBOARD_IRQ		1
#define CASCADE_IRQ		2		// valid cascade for 2nd AT controller
#define ETHER_IRQ		3		// default ethernet interrupt vector
#define SECONDARY_IRQ		3		// RS232 interrupt vector of port 2
#define RS232_IRQ		4		// RS232 interrupt vector of port 1
#define XT_WINI_IRQ		5		// xt winchester
#define FLOPPY_IRQ		6		// floppy disk
#define PRINTER_IRQ		7
#define AT_WINI_IRQ		14		// AT winchester

// interrupt number to hardware vector
#define BIOS_VECTOR(irq)	(((irq) < 8 ? BIOS_IRQ0_VEC : BIOS_IRQ8_VEC) + ((irq) & 0x07))
#define VECTOR(irq)		(((irq) < 8 ? IRQ0_VECTOR : IRQ8_VECTOR) + ((irq) & 0x07))

// BIOS hardware disk parameter vector
#define WINI_0_PARM_VEC		0x41
#define WINI_1_PARM_VEC		0x46

// 8259A interrupt controller port
#define INT_CTL			0x20		// I/O port for interrupt controller
#define INT_CTLMASK		0x21		// setting bit to disable interrupts on this port
#define INT2_CTL		0xA0		// I/O port for secondary interrupt controller
#define INT2_CTLMASK		0xA1		// setting bit to disable interrupts on this port

// magic number of interrupt controller
#define ENABLE			0x20		// after interrupt,code used to re-enable

// size of memory table
#define NR_MEMS			3		// number of memory chunks

// other port
#define PCR			0x65		// planer control register
#define PORT_B			0x61		// I/O port of 8255 port B(keyboard,mother...)
#define TIMER0			0x40		// I/O port of timer channel 0
#define TIMER2			0x42		// I/O port of timer channel 2
#define TIMER_MODE		0x43		// I/O port of timer mode control

#endif // (CHIP == INTEL)

#if (CHIP == M68000)

#define K_STACK_BYTES		1024		// number of bytes of kernel stack

// size of memory table
#define NR_MEMS			2		// number of memory chunks

// p_reg includes d0-d7,a0-a6 in this order
#define NR_REGS			15		// number of general registers in each process slot

#define TRACEBIT		0x8000		// take psw and OR of proc[] and use them for tracing
#define SETPSW(rp,new)		/* allow only certain bits to be set */ \
	((rp)->p_reg.psw = (rp)->p_reg.psw & 0xFF | (new) & 0xFF)
#define MEM_BYTES		0xffffffff	// memory size of /dev/mem

#ifdef __ACK__
#define FSTRUCOPY
#endif

#endif // (CHIP == M68000)

// the following items are related to the scheduling queue.
#define TASK_0			0		// execution town tasks are scheduled in queue 0
#define TASK_1			1		// execution town tasks are scheduled in queue 1
#define TASK_2			2		// execution town tasks are scheduled in queue 2

#if (MACHINE == ATARI)
#define SHADOW_Q		3		// executable,but shadow process
#define NQ			4		// number of scheduling queue
#else
#define NQ			3		// number of scheduling queue
#endif // (CHIP == ATARI)

// Env_parse() return values.
#define EP_UNSET		0		// variables that are not set
#define EP_OFF			1		// var = off
#define EP_ON			2		// var = on
#define EP_SET			3		// var = 1:2:3(without blank field)

// replace the kernel space address with the physical address.
// this is the same as umap(proc_ptr,D,vir,sizeof(*vir)) , but at a much lower cost.
#define vir2phys(vir)		(data_base + (vir_bytes) (vir))
#define printf			printk		// the kernel actually uses printk instead of printf

