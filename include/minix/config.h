#ifndef _CONFIG_H
#define _CONFIG_H

// minix release and version number
#define OS_RELEASE	"0.0"
#define OS_VERSION	"0"

// <minix/config.h> header is the first file that is included in all parts of
// the master header and is actually processed by the compiler.
//
// in many cases, differences in hardware or operating system usage will require
// changes to the minix configuration, and you will have to edit this file
// and recompile your system.
// 
// all user-configurable parameters are in the first part of the file.
//
//
// this file sets the configuration parameters for the minix kernel,FS,MM.
// devided into two main sections.
// the first section contains user-configurable parameters.
// the second section sets various internal system parameters based on user-configurable parameters.
//
//
//	======================================================================================
//
//		     the following sections contain user-configurable parameters.
//
//	======================================================================================
#define MACHINE		IBM_PC		// must have the following name

#define IBM_PC		1		// system of 8088 or 80x86 base
#define SUN_4		40		// system of Sun SPARC base
#define SUN_4_60	40		// Sun-4/60(or SparcStation 1 or Campus)
#define ATARI		60		// ATARI ST/STe/TT (68000/68030)
#define AMIGA		61		// Coommodore Amiga(68000)
#define MACINTOSH	62		// Apple Machintosh(68000)

// word size of byte unit ( constant equal to sizeof(int))
#if __ACK__
#define _WORD_SIZE	_EM_WSIZE
#endif	// __ACK__

// if ROBUST is set to 1, joint block writes from inodes,directories,and caches occur
// as soon as the block changes.
// this makes the file system even stronger, but slower.
// when ROBUST is set to 0,no special action is taken on these blocks,
// which can cause problems if the system crashes.
#define ROBUST		0		// 0 for speed, 1 for strength

// numberr of slots in the process table for user process.
#define NR_PROCS	32

// the buffer cache should be as large as possible
#if (MACHINE == IBM_PC && _WORD_SIZE == 2)
#define NR_BUFS		40		// maximum number of blocks in the buffer cache
#define NR_BUF_HASH	64		// size of buffer hash table; power of 2
#endif // (MACHINE == IBM_PC && _WORD_SIZE == 2)

#if (MACHINE == IBM_PC && _WORD_SIZE == 4)
#define NR_BUFS		512		// maximum number of blocks in the buffer cache
#define NR_BUF_HASH	1024		// size of buffer hash table; power of 2
#endif // (MACHINE == IBM_PC && _WORD_SIZE == 4)

#if (MACHINE == SUN_4_60)
#define NR_BUFS		512		// maximum number of blocks in the buffer cache
#define NR_BUF_HASH	512		// size of buffer hash table; power of 2
#endif // (MACHINE == SUN_4_60

#if (MACHINE == ATARI)
#define NR_BUFS		1536		// maximum number of blocks in the buffer cache
#define NR_BUF_HASH	2048		// size of buffer hash table; power of 2
#endif // (MACHINE == ATARI)

// definition of kernel configuration
#define AUTO_BIOS		0	// xt_wini.c - user autoconfig BIOS of Western
#define LINEWRAP		1	// console.c - wrap a line with 80 columns
#define ALLOW_GAP_MESSAGES	1	// proc.c - allows message for gaps between the end of bss and the bottom of the stack address

// enable or disable the second level file system cache on the RAM disk
#define ENABLE_CACHE2		0

// include or exclude device drivers.
// setting 1 for include, setting 0 for exclude.
#define ENABLE_NETWORKING	0	// enable TCP/IP code
#define ENABLE_AT_WINI		1	// enable AT Winchester driver
#define ENABLE_BIOS_WINI	0	// enable BIOS Winchester driver
#define ENABLE_ESDI_WINI	0	// enable ESDI Winchester driver
#define ENABLE_XT_WINI		0	// enable XT Winchester driver
#define ENABLE_ADAPTEC_SCSI	0	// enable ADAPTEC SCSI driver
#define ENABLE_MITSUMI_CDROM	0	// enable Mitsumi CD-ROM driver
#define ENABLE_SB_AUDIO		0	// enable Soundblaster audio driver

// DMA_SECTORS can be increased to speed up DMA-based drivers.
#define DMA_SECTORS		1	// DMA buffer size(must be >= 1)

// whethere to include or exclude backwards compatibility code
#define ENABLE_BINCOMPAT	0	// for binaries that use calls that are no longer in use
#define ENABLE_SRCCOMPAT	0	// for sources that use calls that are no longer in use

// decide which device to use for the pipe
#define PIPE_DEV		ROOT_DEV	// apply the pipe to the root device

// NR_CONS,NR_RS_LINES,and NR_PTYS determine the number of terminals the system can handle.
#define NR_CONS			2	// system console number (1~8)
#define NR_RS_LINES		0	// rs232 terminal number (0,1,2)
#define NR_PTYS			0	// pseudo terminal number (0~64)

#if (MACHINE == ATARI)
// nett is defined as having ATARI ST or TT
#define ATARI_TYPE		TT
#define ST			1	// all ST and Mega ST
#define STE			2	// all STe and Mega STe
#define TT			3

//  graphical screen operation is possible when SCREEN is set to 1.
#define SCREEN			1

// defined whether the keyboard generates a VT100,or IBM_PC escape.
#define KEYBOARD		VT100	// VT100 or IBM_PC
#define VT100			100

// the type of partitioning is defined below
#define PARTITIONING 		SUPRA	// one of the following, or ATARI
#define SUPRA			1	// ICD,SUPRA and BMS are all the same
#define BMS			1
#define ICD			1
#define CBHD			2
#define EICKMANN		3

// define the number of hard disk drives in your system
#define NR_ACSI_DRIVES		3	// normally 0 or 1
#define NR_SCSI_DRIVES		1	// normally 0(ST,STe) or 1(TT)

// some systems require a slight delay after each Winchester command.
// such systems need to set FAST_DISK to 0.
// other disks do not require this delay,so set FAST_DISK to 1 to cap this delay.
#define FAST_DISK		1	// 0 or 1

// if you want to make the kernel smaller,you can set NR_FD_DRIVES to 0.
// again,you can boot minix.img from a floppy.
// however,both the root and the user file system must be on the hard disk.

// define floppy disk drives number of system
#define NR_FD_DRIVES		1	// 0,1,2

// this configuration defines parallel printer code control
#define PAR_PRINTER		1	// disable(0) or enable(1) parallel printing

// this configuration defines the clock code control of the disk controller.
#define HD_CLOCK		1	// disable(0) or enable(1) hard disk clock

#endif // (MACHINE == ATARI)

//
// ==============================================================================================
//
//		there are no user-configurable parameters after this line
//
// ==============================================================================================
// set the CHIP type based on the washed machine.
// the symbol CHIP actually means more than just a CPU.
// for example, a machine with CHIP==INTEL is generally said to have
// the characteristics of an IBM PC/XT/AT/386 type machine,such as an 8259A interrupt controller.
#define INTEL			1	// CHIP type for PC,XT,AT,386 and clone
#define M68000			2	// CHIP type for Atari,Amiga,Macintosh
#define SPARC			3	// CHIP type for SUN_4(example,SPARCstation)

// set FP_FORMAT type, either hard or soft, based on the selected machine
#define FP_NONE			0	// without supporting float
#define FP_IEEE			1	// apply IEEE floating point standard

#if (MACHINE ==  IBM_PC)
#define CHIP			INTEL
#define SHADOWING		0
#define ENABLE_WINI		(ENABLE_AT_WINI || ENABLE_BIOS_WINI || ENABLE_ESDI_WINI || ENABLE_XT_WINI)
#define ENABLE_SCSI		(ENABLE_ADAPTEC_SCSI)
#define ENABLE_CDROM		(ENABLE_MITSUMI_CDROM)
#define ENABLE_AUDIO		(ENABLE_SB_AUDIO)

#if (MACHINE == ATARI) || (MACHINE == AMIGA) || (MACHINE == MACINTOSH)
#define CHIP			SPARC
#define FP_FORMAT		FP_IEEE
#define SHADOWING		0
#endif // (MACHINE == ATARI) || (MACHINE == AMIGA) || (MACHINE == MACINTOSH)

#if (MACHINE == SUN_4) || (MACHINE == SUN_4_60)
#define CHIP			SPARC
#define FP_FORMAT		FP_IEEE
#define SHADOWING		0
#endif // (MACHINE == SUN_4) || (MACHINE == SUN_4_60)

#if (MACHINE == ATARI) || (MACHINE == SUN_4)
#define ASKDEV			1	// request a boot device
#define FASTLOAD		1	// use multiple block transfers for memory initialication
#endif // (MACHINE == ATARI) || (MACHINE == SUN_4)

#if (ATARI_TYPE == TT) // everything except 68030
#define FPP
#undef SHADOWING
#define SHADOWING		0
#endif // (ATARI_TYPE == TT)

#ifndef FP_FORMAT
#define FP_FORMAT		FP_NONE
#endif // FP_FORMAT

// buf.h use MAYBE_WRITE_IMMED
#if  ROBUST
#define MAYBE_WRITE_IMMED	WRITE_IMMED	// it will be slower,but it seems to be safer
#else
#define MAYBE_WRITE_IMMED	0		// be faster
#endif // ROBUST

#ifndef MACHINE
error "In <minix/config.h> please define MACHINE"
#endif // MACHINE

#ifndef CHIP
error "In <minix/config.h> please define MACHINE to have a legal value"
#endif // CHIP

#if (MACHINE == 0)
error "MACHINE has incorrect value (0)"
#endif

#endif // _CONFIG_H
