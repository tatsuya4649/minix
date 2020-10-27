// <minix/com.h> header that includes a shared definition used to send message from MM ans FS
// to I/O tasks, and each task number.
//
// task number has a negative value to distinguish it from the process number.
//

// system call
#define SEND		1			// function code for sending message
#define RECEIVE		2			// function code for receiving message
#define BOTH		3			// function code for SEND+RECEIVE
#define ANY		(NR_PROCS + 100)	// receive(ANY,buf) is accepted from any source

// task number,function code,and answer code

// the value of some task numbers is determined by whether those tasks or other tasks are
// enabled. such a task is commonly defined as (PREVIOUUS_TASK --ENABLE_TASK).
// ENABLE_TASK is 0 or 1, and task gets a new number or the same number as the previous
// task, but it is still not used.
// TTY task always has a negative minimum task number.therefore,it should be 
// initialized first.
// many of TTY function code are shared with other tasks.
#define TTY		(DL_ETH - 1) 		
						// terminal I/O class
#define CANCEL		0			// general request for forced task cancellation
#define HARD_INT	2			// function code for all hardware interrupts
#define DEV_READ	3			// function code for reading from tty
#define DEV_WRITE	4			// function code for writing to tty
#define DEV_IOCTL	5			// function code for ioctl
#define DEV_OPEN	6			// function code for opening tty
#define DEV_CLOSE	7			// function code for closing tty
#define SCATTERED_IO	8			// multiple read/write function code
#define TTY_SETPGRP	9			// function code for setpgroup
#define TTY_EXIT	10			// process group leader exited
#define OPTIONAL_IO	16			// DEV_* code modifier in the vector
#define SUSPEND		-998			// if no data in tty,used in interrupt

#define DL_ETH		(CDROM - ENABLE_NETWORKING)	// networking task

// data link layer request message type
#define DL_WRITE	3
#define DL_WRITEV	4
#define DL_READ		5
#define DL_READV	6
#define DL_INIT		7
#define DL_STOP		8
#define DL_GETSTAT	9

// data link layer answer message type
#define DL_INIT_REPLY	20
#define DL_TASK_REPLT	21

#define DL_PORT		m2_i1
#define DL_PROC		m2_i2
#define DL_COUNT	m2_i3
#define DL_MODE		m2_l1
#define DL_CLCK		m2_l2
#define DL_ADDR		m2_p1
#define DL_STAT		m2_l1

// bits in the 'DL_STAT' field of DL answer
#define DL_PACK_SEND	0x01
#define DL_PACK_RECV	0x02
#define DL_READ_IP	0x04

// bits in the 'DL_STAT' field of DL request
#define DL_NOMODE	0x0
#define DL_PROMISC_REQ	0x2
#define DL_MULTI_REQ	0x4
#define DL_BROAD_REQ	0x8

#define NW_OPEN		DEV_OPEN
#define NW_CLOSE	DEV_CLOSE
#define NW_READ		DEV_READ
#define NW_WRITE	DEV_WRITE
#define NW_IOCTL	DEV_IOCTL
#define NW_CANCEL	CANCEL

#define CDROM		(AUDIO - ENABLE_CDROM)		// cd-rom device task

#define AUDIO		(MIXER - ENABLE_AUDIO)
#define MIXER		(SCSI - ENABLE_AUDIO)		// audio & mixier device task


#define SCSI		(WINCHESTER - ENABLE_SCSI)	// scsi device task

#define WINCHESTER	(SYN_ALRM_TASK - ENABLE_WINI)	// winchester(hard) disk class

#define SYN_ALRM_TASK	-8				// CLOCK_INT message send task
#define IDLE		-7				// tasks to perform when there is nothing to do 
#define PRINTER		-6				// printer I/O class
#define FLOPPY		-5				// floppy disk class
#define MEM		-4				// /dev/ram,/dev/(k)mem and /dev/null class
#	define NULL_MAJOR	1			// mejor device for /dev/null
#	define RAM_DEV		0			// minor device for /dev/ram
#	define MEM_DEV		1			// minor device for /dev/mem
#	define KMEM_DEV		2			// minor device for /dev/kmem
#	define NULL_DEV		3			// minor device for /dev/null

#define CLOCK		-3				// clock class
#	define SET_ALARM	1			// function code of CLOCK, setting alarm
#	define GET_TIME		3			// function code of CLOCK, getting realtime
#	define SET_TIME		4			// function code of CLOCK, setting realtime
#	define GET_UPTIME	5			// function code of CLOCK, getting uptime
#	define SET_SYNC_AL	6			// function code of CLOCK, alarm setting that times out on transmission
#define REAL_TIME		1			// answer from CLOCK : following is realtime
#define CLOCK_INT		HARD_INT		// code sent by SYN_ALRM_TASK for the task that requested the sync alarm


#define SYSTASK		-2				// internal function
#	define SYS_XIT		1			// function code for sys_xit(parent,proc)
#	define SYS_GETSP	2			// function code for sys_sp(proc,&new_sp)
#	define SYS_OLDSIG	3			// function code for sys_oldsig(proc,sig)
#	define SYS_FORK		4			// function code for sys_fork(parent,child)
#	define SYS_NEWMAP	5			// function code for sys_newmap(procno,map_ptr)
#	define SYS_COPY		6			// function code for sys_copy(ptr)
#	define SYS_EXEC		7			// function code for sys_exec(procno,new_sp)
#	define SYS_TIMES	8			// function code for sys_times(procno,bufptr)
#	define SYS_ABORT	9			// function code for sys_abort()
#	define SYS_FRESH	10			// function code for sys_fresh()
#	define SYS_KILL		11			// function code for sys_kill(proc,sig)
#	define SYS_GBOOT	12			// function code for sys_gboot(procno,bootptr)
#	define SYS_UMAP		13			// function code for sys_umap(procno,etc)
#	define SYS_MEM		14			// function code for sys_mem()
#	define SYS_TRACE	15			// function code for sys_trace(req,pid,addr,data)
#	define SYS_VCOPY	16			// function code for sys_vcopy(src_proc,dest_proc,vcopy_s,Vcopy_ptr)
#	define SYS_SENDSIG	17			// function code for sys_sendsig(&sigmsg)
#	define SYS_SIGRETURN	18			// function code for sys_sigreturn(&sigmsg)
#	define SYS_ENDSIG	19			// function code for sys_endsig(procno)
#	define SYS_GETMAP	20			// function code for sys_getmap(procno,map_ptr)

#define HARDWARE		-1			// used as source in interrupt-generated msgs

// message field name for message to CLOCK task
#define DELTA_TICKS		m6_l1			// alarm interval in clock units
#define FUNC_TO_CALL		m6_f1			// pointer calling function
#define NEW_TIME		m6_l1			// value to set the clock to (SET_TIME)
#define CLOCK_PROC_NR		m6_i1			// alarm required by proc(or task)
#define SECONDS_LEFT		m6_l1			// number of seconds remainig

// message used for block and character tasks message field name
#define DEVICE			m2_i1			// major - minior device
#define PROC_NR			m2_i2			// I/O required in (proc)
#define COUNT			m2_i3			// number of bytes transferred
#define REQUEST			m2_i3			// ioctel request code
#define POSITION		m2_l1 			// file offset
#define ADDRESS			m2_p1			// core buffer address

// field name for message to TTY task
#define TTY_LINE		DEVICE			// message parameter : terminal line
#define TTY_REQUEST		COUNT			// message parameter : ioctl required code
#define TTY_SPEK		POSITiON		// message parameter : ioctl spped,delete
#define TTY_FLAGS		m2_l2			// message parameter : ioctl tty mode
#define TTY_PGRP		m2_i3			// message parameter : proccess group

// QIC 02 status answer message field name from tape driver
#define TAPE_STAT0		m2_l1
#define TAPE_STAT1		m2_l2

// answer message field name from task
#define REP_PROC_NR		m2_i1			// proc number where I/O was done
#define REP_STATUS		m2_i2			// number of bytes transferred or number of error

// copy message field to SYSTASK
#define SRC_SPACE		m5_c1			// space of T or D(stack too D)
#define SRC_PROC_NR		m5_i1			// process of copy source
#define SRC_BUFFER		m5_l1			// virtual address of data source
#define DST_SPACE		m5_c2			// T or D space (stack too D)
#define DST_PROC_NR		m5_i2			// copy destination process
#define DST_BUFFER		m5_l2			// virtual address of sending destination of data
#define COPY_BYTES		m5_l3			// number of copy bytes

// accounting,SYSTASK etc field name
#define USER_TIME		m4_l1			// user time consumed by the process
#define SYSTE_TIME		m4_l2			// syste time cosumed by the process
#define CHILD_UTIME		m4_l3			// user time consumed by child process of process
#define CHILD_STIME		m4_l4			// system time consumed by child process of process
#define BOOT_TICKS		m4_l5			// number of clock tick since boot
#define PROC1			m1_i1			// show process
#define PROC2			m1_i2			// show process
#define PID			m1_i3			// process ID passed from MM to kernel
#define STACK_PTR		m1_p1			// used for sys_exec,sys_getsp stack pointer
#define PR			m6_i1			// number of process of sys_sig
#define SIGNUM			m6_i2			// number of signal of sys_sig
#define FUNC			m6_i3			// function pointer of sys_sig
#define MEM_PTR			m1_p1			// indicate the location of sys_newmap's memory map
#define NANE_PTR		m1_p2			// indicated the location of the program name for dmp
#define IP_PTR			m1_p3			// initial value of ip after exec
#define SIG_PROC		m2_i1			// process number to notify
#define SIG_MAP			m2_l1			// used by kernel to pass signal bitmaps
#define SIG_MSG_PTR		m1_i1			// a pointer to information for building a signal acquisition stack
#define SIG_CTXT_PTR		m1_p1			// a pointer to information for recovering the signal context


