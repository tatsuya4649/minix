// <minix/const.h> header define various macros used in many places
//
//

#define EXTERN		extern		// used in *.h file
#define PROVATE		static		// PRIVATE x limit range of x
#define PUBLIC				// PUBLIC is the opposite of PRIVATE
#define FORWARD		static		// some compilers require this to be static

#define TRUE		1		// used to make int boolean
#define FALSE		0		// used to make int boolean

#define HZ		60		// clock frequency
#define BLOCK_SIZE	1024		// byte number of disk block
#define SUPER_USER   (uid_t) 0		// uid_t of superuser

#define MAJOR		8		// major device = (dev>>MAJOR)&0377
#define MINOR		0		// minor device = (dev>>MINOR)&0377

#define NULL		((void*)0)	// null pointer
#define CPVEC_NR	16		// maximum number of entries for SYS_VCOPY request
#define NR_IOREQS	MIN(NR_BUFS,64) // maximum number of entries for I/O request

#define NR_SEGS		3		// number of segment per process
#define T		0		// proc[i].mem_map[T] is for text
#define D		1		// proc[i].mem_map[T] is for data
#define S		2		// proc[i].mem_map[T] is for stack

// list of process number for important processes
#define MM_PROC_NR	0		// process number of memory manager
#define FS_PROC_NR	1		// process number of file system
#define INET_PROC_NR	2		// process number of TCP/IP server
#define INIT_PROC_NR	(INET_PROC_NR + ENABLE_NETWORKING)	// init -- process for multi user
#define LOW_USER	(INET_PROC_NR + ENABLE_NETWORKING)	// first user not part of the operating system

// other
#define BYTE		0377		// for 8bit mask
#define READING		0		// copy data to user
#define WRITING		1		// copy data from user
#define NO_NUM		0x8000		// used as numeric argument to panic()
#define NIL_PTR		(char*) 0 	// generally valid expressions
#define HAVE_SCATTERED_IO	1	// distributed I/O is currently the standard

// macro
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MIN(a,b)	((a) > (b) ? (b) : (a))

// number of task
#define NR_TASKS	(9+ENABLE_WINI+ENABLE_SCSI+ENABLE_CDROM+ENABLE_NETWORKING+2*ENABLE_AUDIO)

// memory is allocated on a click-by-click basic
#if (CHIP==INTEL)
#define CLICK_SIZE	256		// unit to which memory is allocated
#define CLICK_SHIFT	8		// log2 of CLICK_SIZE
#endif // CHIP == INTEL

#if (CHIP == SPARC) || (CHIP == M68000)
#define CLICK_SIZE	4906		// unit to which memory is allocated
#define CLICK_SHIFT	12		// log2 of CLICK_SIZE
#endif // (CHIP == SPARC) || (CHIP == M68000)

#define click_to_round_k(n) ((unsigned) ((((unsigned long) (n) << CLICK_SHIFT) + 512) / 1024))
#if CLICK_SIZE < 1024
#define k_to_click(n) ((n) * (1024 / CLICK_SIZE))
#else
#define k_to_click(n) ((n) * (CLICK_SIZE / 1024))
#endif

#define ABS		-999		// means absolute memory

// flag bit for i_mode in inode
#define I_TYPE		0170000		// indicates inode type
#define I_REGULAR	0100000		// regular files that are neither directories nor special files
#define I_BLOCK_SPECIAL	0060000		// block special file
#define I_DIRECTORY	0040000		// file is directory
#define I_CHAR_SPECIAL	0020000		// character special file
#define I_NAMED_PIPE	0010000		// named pipe (FIFO)
#define I_SET_UID_BIT	0004000		// set a valid uid_t at runtime
#define I_SET_GID_BIT	0002000		// set a valid gid_t at runtime
#define ALL_MODES	0006777		// all bits for users,groups,and more
#define RWX_MODES	0000777		// RWX dedicated mode bit
#define R_BIT		0000004		// Rwx protection bit
#define W_BIT		0000002		// rWx protection bit
#define X_BIT		0000001		// rwX protection bi
#define I_NOT_ALLOC	0000000		// this inode is free

// limit value
#define MAX_BLOCK_NR	((block_t) 077777777) // maximum block number
#define HIGHEST_ZONE	((zone_t) 077777777)  // maximum zone number
#define MAX_INODE_NR	((ino_t) 0177777)     // maximum inode number
#define MAX_FILE_POS	((off_t) 037777777777)// maximum valid file offset

#define NO_BLOCK	((block_t) 0)	// lack of block number
#define NO_ENTRY	((ino_t) 0)	// lack of directory entory
#define NO_ZONE		((zone_t) 0)	// lack of zone number
#define NO_DEV		((dev_t) 0)	// lack of device number

