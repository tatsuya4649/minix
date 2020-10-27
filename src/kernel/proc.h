#ifndef PROC_H
#define PROC_H

// the following is the process table declaration.contains process registers,
// memory maps,accounting,and send/receive information.many assenmbly code
// routines refer to the fields in this.
// offsets for there fields are defined in sconst.h,which is included in the assembler.
// if you change 'proc',you must change sconst.h to match.

struct proc{
	struct stackframe_s p_reg;		// process registers stored in the stack frame
#if (CHIP == INTEL)
	reg_t p_ldt_sel;			// ldt base and gdt selector to indicate limits
	struct segdesc_s p_ldt[2];		// local description of code and deta
						// 2 is LDT_SIZE --- avoid include of protect.h
#endif // (CHIP == INTEL)
	reg_t *p_stguard;			// stack guard word
	int p_nr;				// number of process ( for fast access )
	int p_int_blocked;			// non-zero when int msg is blocked by the task in use
	int p_int_held;				// non-zero when int msg is held by syscall in use
	struct proc *p_nextheld;		// the next chain of hold-up int processes
	int p_flags;				// P_SLOT_FREE,SENFING,RECEIVING,etc...
	struct mem_map p_map[NR_SEGS];		// memory map
	pid_t p_pid;				// process id passed by MM
	clock_t user_time;			// user time (number of clock)
	clock_t	sys_time;			// system time (number of clock)
	clock_t child_utime;			// cumulative user time for child process
	clock_t child_stime;			// cumulative system time for child process
	clock_t p_alarm;			// next alarm time ( number of clock or zero )
	struct proc *p_callerq;			// top of table for sending process
	struct proc *p_sendlink;		// link to the next submit process
	message *p_messbuf;			// pointer of message buffer
	int p_getfrom;				// the recipient the process wants
	int p_sendto;	

	struct proc *p_nextready;		// pointer to the next waiting process
	sigset_t p_pending;			// bitmap of suspension signal
	unsigned p_pendcount;			// number of incomplete signals suspended
	
	char p_name[6];				// name of process
};

// guard name of task stack
#define STACK_GUARD		((reg_t) (sizeof(reg_t) == 2 ? 0xBEEF : 0xDEADBEEF))

// bit of p_flag of proc[]. if p_flag == 0, process is executable.
#define P_SLOT_FREE		001		// set when the slot is not in use
#define NO_MAP			002		// avoiding execution of unmapped branch child processes
#define SENDING			004		// set when process sending is blocked
#define RECEIVING		010		// set when process receiving is blocked
#define PENDING			020		// set when signal inform() is interrupted
#define SIG_PENDING		040		// avoiding execution of signal recipient proc
#define P_STOP			0100		// set when process is traced

// magic process table address
#define BEG_PROC_ADDR		(&proc[0])
#define END_PROC_ADDR		(&proc[NR_TASKS + NR_PROCS])
#define END_TASK_ADDR		(&proc[NR_TASKS])
#define BEG_SERV_ADDR		(&proc[NR_TASKS])
#define BEG_USER_ADDR		(&proc[NR_TASKS + LOW_USER])

#define NIL_PROC		((struct proc *) 0)
#define isidlehardware(n)	((n) == IDLE || (n) == HARDWARE)
#define isokprocn(n)		((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
#define isoksrc_dest(n)		(isokprocn(n) || (n) == ANY)
#define isoksusern(n)		((unsigned) (n) < NR_PROCS)
#define isokusern(n)		((unsigned) ((n) - LOW_USER) < NR_PROCS - LOW_USER)
#define isrxhardware(n)		((n) == ANY || (n) == HARDWARE)
#define issysentn(n)		((n) == FS_PROC_NR || (n) == MM_PROC_NR)
#define istaskp(p)		((p) < END_TASK_ADDR && (p) != proc_addr(IDLE))
#define	isuserp(p)		((p) >= BEG_USER_ADDR)
#define proc_addr(n)		(pproc_addr + NR_TASKS)[(n)]
#define cproc_addr(n)		(&(proc + NR_TASKS)[(n)])
#define proc_number(p)		((p)->p_nr)
#define proc_vir2phys(p,vir) 	(((phys_bytes)(p)->p_map[D].mem_phys << CLICK_SHIFT) + (vir_bytes)(vir))

EXTERN struct proc proc[NR_TASKS + NR_PROCS];	// process table
EXTERN struct proc *pproc_addr[NR_TASKS + NR_PROCS]; // pointer to process table slot.

EXTERN struct proc *bill_ptr; // a pointer to the billing process for the number of clocks
EXTERN struct proc *rdy_head[NQ]; // a pointer to the begginnig of the process table waiting to be executed
EXTERN struct proc *rdy_tail[NQ]; // a pointer to the last of the process table waiting to be executed

#endif // PROC_H
