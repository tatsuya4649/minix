// constant of protection mode
//

// table size
#define GDT_SIZE(FIRST_LDT_INDEX + NR_TASKS + NR_PROCS)	// spec. and LDT
#define IDT_SIZE(IRQ8_VECTOR + 8)			// up to the highest vector
#define LDT_SIZE	2				// include only CD and DS

// fixed global descriptor
#define GDT_INDEX	1			// GDT descriptor
#define IDT_INDEX	2			// IDT descriptor
#define DS_INDEX	3			// kernel DS
#define ES_INDEX	4			// kernel ES(386: flag 4 Gb when start up)
#define SS_INDEX	5			// kernel SS(386: monitorSS when start up)
#define CS_INDEX	6			// kernel CS
#define MON_CS_INDEX	7			// temporary value of BIOS(386: monitorCS when start up)
#define TSS_INDEX	8			// kernel TSS
#define DS_286_INDEX	9			// scratch 16-bit source segment
#define ES_286_INDEX	10			// scratch 16-bit destination segment
#define VIDEO_INDEX	11			// video memory segment
#define DP_ETH0_INDEX	12			// Western Digital Etherplus buffer
#define DP_ETH1_INDEX	13			// Western Digital Etherplus buffer
#define FIRST_LDT_INDEX	14			// the rest of descriptor is LD

#define GDT_SELECTOR	0x08			// (GDT_INDEX * DESC_SIZE)not suitable for asld
#define IDT_SELECTOR	0x10			// (IDT_INDEX * DESC_SIZE)
#define DS_SELECTOR	0x18			// (DS_INDEX * DESC_SIZE)
#define ES_SELECTOR	0x20			// (ES_INDEX * DESC_SIZE)
#define FLAT_DS_SELECTOR 0x21			// ES with low privilege level
#define SS_SELECTOR	0x28			// (SS_INDEX * DESC_SIZE)
#define CS_SELECTOR 	0x30			// (CS_INDEX * DESC_SIZE)
#define MON_CS_SELECTOR	0x38			// (MON_CS_INDEX * DESC_SIZE)
#define TSS_SELECTOR	0x40			// (TSS_INDEX * DESC_SIZE)
#define DS_286_SELECTOR	0x49			// (DS_286_INDEX * DESC_SIZE)
#define ES_286_SELECTOR 0x51			// (ES_286_INDEX * DESC_SIZE)
#define VIDEO_SELECTOR	0x59			// (VIDEO_INDEX * DESC_SIZE)
#define DS_ETH0_SELECTOR 0x61			// (DS_ETH0_INDEX * DESC_SIZE)
#define DS_ETH1_SELECTOR 0x69			// (DS_ETH1_INDEX * DESC_SIZE)

// fixed local descriptor
#define CS_LDT_INDEX	0			// process CS
#define DS_LDT_INDEX	1			// process DS=ES=FS=GS=SS

// privilege
#define INTR_PRIVILEGE	0			// kernel and interrupt handler
#define TASK_PRIVILEGE	1
#define USER_PRIVILEGE	3

// 286 hardware constant

// exception vector number
#define BOUNDS_VECTOR		5		// boundary check failed
#define INVAL_OP_VECTOR		6		// invalid operation code
#define COPROC_NOT_VECTOR	7		// can't use coprocessor
#define DOUBLE_FAULT_VECTOR	8
#define COPROC_SEG_VECTOR	9		// coprocessor segment overrun
#define INVAL_TSS_VECTOR	10		// invalid TSS
#define SEG_NOT_VECTOR		11		// not exsist segment
#define STACK_FAILT_VECTOR	12		// exception stack
#define PROTECTION_VECTOR	13		// general protection

// selector bit
#define TI			0x04		// table descriptor
#define PRL			0x03		// requester privilege level

// descriptor struct offset
#define DESC_BASE		2		// to base_low
#define DESC_BASE_MIDDLE	4		// to base_middle
#define DESC_ACCESS		5		// to access byte
#define DESC_SIZE		8		// sizeof(struct segdesc_s)

// segment size
#define MAX_286_SEG_SIZE	0x1000L

// base and size raneg and shift
#define BASE_MIDDLE_SHIFT	16		// shift of base ---> base_middle

// access bytes and type bytes bit
#define PRESENT		0x80			// set for existing descriptor
#define DPL		0x60			// descriptor privilege level mask
#define DPL_SHIFT	5
#define SEGMENT		0x10			// set for segment type descriptor

// access bytes bit
#define EXECUTABLE	0x08			// set for executable segment
#define CONFORMING	0x04			// set to fit segment if executable
#define EXPAND_DOWN	0x04			// greatly set for extended down segment if not executable
#define READABLE	0x02			// set for readable segment if executable
#define WRITEABLE	0x02			// set for writeable segment if not executable
#define TSS_BUSY	0x02			// set if TSS descriptor is used in
#define ACCESSED	0x01			// set if segment is accessed

// special descriptor
#define AVL_286_TSS	1			// abailable 286 TSS
#define LDT		2			// local descriptor table
#define BUSY_286_TSS	3			// set so that it can be referenced from software
#define CALL_286_GATE	4			// unuse
#define TASK_GATE	5			// used by debugger
#define INT_286_GATE	6			// interrupt gate,used for all vectors
#define TRAP_286_GATE	7			// unuse

// spare 386 hardware constant

// exception vector number
#define PAGE_FAULT_VECTOR	14
#define COPROC_ERR_VECTOR	16		// coprocesser error

// descriptor struct offset
#define DESC_GRANULARITY	6		// to fine-grained bytes
#define DESC_BASE_HIGH		7		// to base-high

// base and size range and shift
#define BASE_HIGH_SHIFT		24		// shift of base --> base_high
#define BYTE_GRAN_MAX		0xFFFFFL	// max size of bites fine-grained segment
#define GRANULARITY_SHIFT	16		// shift of range --> fine-grained
#define OFFSET_HIGH_SHIFT	16		// shift of (gate) offset --> offset_high
#define PAGE_GRAN_SHIFT		12		// pare shift of page fine-grained range

// type byte bit
#define DESC_386_BIT		0x08		// don't need LDT and TASK_GATE,which can get
						// 386 bytes by OR with this.

// fine-grained byte
#define GRANULAR		0x80		// set for 4k fine-grained
#define DEFAULT			0x40		// set to 32 bits when omitted(executable segment)
#define BIG			0x40		// set for "BIG"(extended down segment)
#define AVL			0x10		// available
#define LIMIT_HIGH		0x0F		// high-order bitmask of range

