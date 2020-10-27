
#ifndef TYPE_H
#define TYPE_H

typedef _PROTOTYPE(void task_t,(void));
typedef _PROTOTYPE(int (*rdwt_t),(message *m_ptr));
typedef _PROTOTYPE( void (*watchdog_t),(void));

struct tasktab{
	task_t *initial_pc;
	int stksize;
	char name[8];
};

struct memory{
	phys_clicks base;
	phys_clicks size;
};

// clock poling management
struct milli_state{
	unsigned long accum_cout; 	// cumulative clock count
	unsigned prev_count;		// number of clocks immediately before
};

#if (CHIP == INTEL)
typedef unsigned port_t;
typedef unsigned sefm_t;
typedef unsigned reg_t;			// machine register

// the stack frame layout is determined by the software,but for efficiency,
// the assembly code that uses it is specified to be as simple as possible.
// the 80286 protection mode and all real modes use the same frame  built witth 16-bit
// registers. real mode does not have an automatic stack switch,so ti differs only
// in having 32-bit registers and more segment registers.
// use the same name for large registers to avoid differences in the code.
struct stackframe_s{			// proc_ptr points here
#if _WORD_SIZE == 4
	u16_t	gs;			// last item pushed by same
	u16_t	fs;			// ^
#endif // _WORD_SIZE == 4
	u16_t	es;			// |
	u16_t	ds;			// |
	reg_t	di;			// di to cx are not accessed in C
	reg_t	si;			// order fits pusha/popa
	reg_t	fp;			// bp
	reg_t	st;			// free space for another copy of sp
	reg_t	bx;			// |
	reg_t 	dx;			// |
	reg_t	cx;			// |
	reg_t	retreg;			// all ax and above are pushed by save
	reg_t	retadr;			// return address of assembly code save()
	reg_t	pc;			// |
	reg_t	cs;			// |
	reg_t	psw;			// |
	reg_t	sp;			// |
	reg_t	ss;			// these are pushed by the CPU during interrupts
};

struct segdesc_s{			// protected mode segment descriptor
	u16_t	limit_low;
	u16_t	base_low;
	u8_t	base_middle;
	u8_t	access;			// |P|DL|1|X|E|R|A|
#if _WORD_SIZE == 4
	u8_t	granularity;		// |G|X|O|A|LIMT|
	u8_t	base_high;
#else
	u16_t	reserved;
#endif // _WORD_SIZE == 4
};

typedef _PROTOTYPE(int (*irq_handler_t),(int irq));

#endif // (CHIP == INTEL)

#if (CHIP == M68000)
typedef _PROTOTYPE(void (*dmaint_t),(void));

typedef u32_t	reg_t;			// machine register

// the name and fields of this structure were chosen for PC compatibility.
struct stackframe_s{
	reg_t	retreg;
	reg_t	d1;
	reg_t	d2;
	reg_t	d3;
	reg_t	d4;
	reg_t	d5;
	reg_t	d6;
	reg_t	d7;
	reg_t	a0;
	reg_t	a1;
	reg_t	a2;
	reg_t	a3;
	reg_t	a4;
	reg_t	a5;
	reg_t 	fp;
	reg_t	sp;
	reg_t	pc;
	u16_t	psw;
	u16_t	dummy;			// make size a multiple of system.c and reg_t
};

struct fsave{
	struct cpu_state{
		u16_t	i_format;
		u32_t	i_addr;
		u16_t	i_state[4];
	} cpu_state;
	struct state_frame{
		u8_t	frame_type;
		u8_t	frame_size;
		u16_t	reserved;
		u8_t	frame[212];
	} state_frame;
	struct fpp_mode{
		u32_t	fpcr;
		u32_t	fpsr;
		u32_t	fpiar;
		struct fpN{
			u32_t	high;
			u32_t	low;
			u32_t	mid;
		} fpN[8];
	} fpp_mode;
};
#endif // (CHIP == M68000)

#endif // TYPE_H
