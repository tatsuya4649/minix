//  types and constants shared between the generic and device dependent
//  device driver code.
//
//

#include <minix/callnr.h>
#include <minix/com.h>
#include "proc.h"
#include <minix/partition.h>

// info about and entry points into the device dependent code.
struct driver{
	_PROTOTYPE( char *(*dr_name),(void));
	_PROTOTYPE( int (*dr_open), (struct driver *dp,message *m_ptr));
	_PROTOTYPE( int (*dr_close), (struct driver *dp,message *m_ptr));
	_PROTOTYPE( int (*dr_ioctl), (struct driver *dp,message *m_ptr));
	_PROTOTYPE( struct device *(*dr_prepare), (int device));
	_PROTOTYPE( int (*dr_shedule), (int proc_nr,struct iorequest_s *request));
	_PROTOTYPE( int (*dr_finish), (void));
	_PROTOTYPE( void (*dr_cleanup), (void));
	_PROTOTYPE( void (*dr_geometry), (struct partition *entry));
};

#if (CHIP == INTEL)

// number of bytes you can DMA before hitting a 64K boundary
#define dma_bytes_left(phys)	\
	((unsigned) (sizeof(int) == 2 ? 0 : 0x1000) - (unsigned) ((phys) & 0xFFFF))


#endif // CHIP == INTEL

// base and size of a partition in bytes
struct device{
	unsigned long dv_base;
	unsigned long dv_size;
};

#define NIL_DEV		((struct device *)0)

// functions defined by driver.c
_PROTOTYPE( void driver_task, (struct driver *dr));
_PROTOTYPE( int do_rdwt, (struct driver *dr, message *m_ptr));
_PROTOTYPE( int do_vrdwt, (struct driver *dr, message *m_ptr));
_PROTOTYPE( char *no_name, (void));
_PROTOTYPE( int do_nop, (struct driver *dp,message *m_ptr));
_PROTOTYPE( int nop_finish, (void));
_PROTOTYPE( void nop_cleanup, (void));
_PROTOTYPE( void clock_mess, (int ticks, watchdog_t func));
_PROTOTYPE( int do_diocntl, (struct driver *dr,message *m_ptr));

// parameters for the disk driver
#define SECTOR_SIZE	512		// physical sector size in bytes
#define SECTOR_SHIFT	9		// for division
#define SECTOR_MASK	511		// and remainder

// size of the DMA buffer in bytes
#define DMA_BUF_SIZE	(DMA_SECTORS * SECTOR_SIZE)

#if (CHIP == INTEL)
extern u8_t *tmp_buf;			// DMA buffer
#else
extern u8_t tmp_buf[];			// DMA buffer
#endif // CHIP == INTEL
extern phys_bytes tmp_phys;		// phys address of DMA buffer

