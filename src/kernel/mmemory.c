// this file contains the device dependent part of the drivers for the
// following special files:
// 	/dev/null	- null device (data sink)
// 	/dev/mem	- absolute memory
// 	/dev/kmem	- kernel virtual memory
// 	/dev/ram	- RAM disk
//
// the file contains one entry point:
//
// 	mem_task	: main entry when system is brought up
//
#include "kernel.h"
#include "driver.h"
#include <sys/ioctl.h>

#define NR_RAWS		4		// number of RAM-type devices

PRIVATE struct device m_geom[NR_RAWS];	// base and size of each RAM disk
PRIVATE int	m_device;		// current device

FORWARD _PROTOTYPE( struct device *m_prepare,(int device));
FORWARD _PROTOTYPE( int m_schedule,(int proc_nr,struct iorequest_s *iop));
FORWARD _PROTOTYPE( int m_do_open,(struct driver *dp,message *m_ptr));
FORWARD _PROTOTYPE( void m_init,(void));
FORWARD _PROTOTYPE( int m_ioctl,(struct device *dp,message *m_ptr));
FORWARD _PROTOTYPR( void m_geometry,(struct partition *entry));

// entry point to this driver
PRIVATE struct driver m_dtab = {
	no_name,	// current device's name
	m_do_open,	// open or mount
	do_nop,		// nothing on a close
	m_ioctl,	// specify ran disk geometry
	m_prepare,	// prepare for I/O on a given minor device
	m_transfer, 	// do the I/O
	nop_cleanup,	// no need to clean up
	m_gepmetry,	// memory device "geometry"
};

/*======================================================================================*
 * 					mem_task					*
 *======================================================================================*/
PUBLIC void mem_task()
{
	m_init();
	driver_task(&m_dtab);
}


/*======================================================================================*
 * 					m_prepare					*
 *======================================================================================*/
PRIVATE struct device *m_prepare(device)
int device;
{
	// prepare I/O of device
	if (device < 0 || device >= NR_RAWS) return(NIL_DEV);
	return (&m_geom[device]);
}
/*======================================================================================*
 * 					m_schedule					*
 *======================================================================================*/
PRIVATE int m_schedule(proc_nr,iop)
int proc_nr;			// process doing the request
struct iorequest_s *iop;	// a pointer to a read or write request
{
	// read or write /dev/null,/dev/mem,/dev/kmem, or /dev/ram
	int device,count,opcode;
	phys_bytes mem_phys,user_phys;
	struct device *dv;

	// requested type
	opcode = iop->io_request & ~OPTIONAL_IO;

	// get minor device number, check /dev/null
	device = m_device;
	dv = &m_geom[device];

	// determine the destination or source address of the data
	user_phys = numap(proc_nr,(vir_bytes) iop->io_buf,
			(vir_bytes) iop->io_nbytes);
	if (user_phys == 0) return(iop->io_nbytes = EINVAL);

	if (device == NULL_DEV){
		// /dev/null : black hole
		if (opcode == DEV_WRITE) iop->io_nbytes = 0;
		count = 0;
	}else{
		// /dev/mem,/dev/kmem, or /dev/ram : check EOF
		if (iop->io_position >= dv->dv_size) return (OK);
		count = iop->io_nbytes;
		if (iop->io_position + count > dv->dv_size)
			count = dv->dv_size - iop->io_position;
	}
	// set 'mem_phys' for /dev/mem,/dev/kmen, or /dev/ram
	mem_phys = dv->dv_base + iop->io_position;

	// register the number of bytes to be transferred in advance
	iop->io_nbytes -= count;
	if (count == 0) return (OK);

	// copy data
	if (opcode == DEV_READ)
		phys_copy(mem_phys,user_phys,(phys_bytes) count);
	else
		phys_copy(user_phys,mem_phys,(phys_bytes) count);
	return (OK);
}
/*======================================================================================*
 * 					m_do_open					*
 *======================================================================================*/
PRIVATE int m_do_open(dp,m_ptr)
struct driver *dp;
message *m_ptr;
{
	// check device number on open. give I/O privileges to a process opening
	// /dev/mem/ or /dev/kmen
	if (m_prepare(m_ptr->DEVICE) == NIL_DEV) return (ENXIO);

	if (m_device == MEM_DEV || m_device == KMEM_DEV)
		enable_iop(proc_addr(m_ptr->PROC_NR));
	return (OK);
}

/*======================================================================================*
 * 					m_init						*
 *======================================================================================*/
PRIVATE void m_init()
{
	// initialize this task
	extern int _end;


	m_geom[KMEM_DEV].dv_base = vir2phys(0);
	m_geom[KMEM_DEV].dv_size = vir2phys(&_end);

#if (CHIP == INTEL)
	if (!protected_mode){
		m_geom[MEM_DEV].dv_size = 0x100000;			// 1M when 8086 system
	}else{
#if _WORD_SIZE == 2
		m_geom[MEM_DEV].dv_size = 0x1000000;			// 16M when 286
#else
		m_geom[MEM_DEV].dv_size = 0xFFFFFFFF;			// 4G-1 when 386 system
#endif
	}
#endif // CHIP == INTEL
}
/*======================================================================================*
 * 					m_ioctl						*
 *======================================================================================*/
PRIVATE int m_ioctl(dp,m_ptr)
struct driver *dp;
message *m_ptr;				// pointer to message to read or write
{
	// set parameters for one of the RAM disks
	unsigned long bytesize;
	unsigned base,size;
	struct memory *memp;
	static struct psinfo psinfo = { NR_TASK,NR_PROCS,(vir_bytes) proc,0,0};
	phys_bytes psinfo_phys;

	switch(m_ptr->REQUEST){
		case MIOCRAMSIZE:
			// FS set RAM disk size
			if (m_ptr->PROC_NR != FS_PROC_NR) return(EPERM);
			
			bytesize = m_ptr->POSITION * BLOCK_SIZE;
			size = (bytesize + CLICK_SHIFT - 1) >> CLICK_SHIFT;
			
			// find a memory chunk big enough for the RAM disk
			memp = &mem[NR_MEMS];
			while ((--memp)->size < size){
				if (memp==mem) panic("RAM disk is too big",NO_NUM);
			}
			base = memp->base;
			memp->base += size;
			memp->size -= size;
			
			m_geom[RAM_DEV].dv_base = (unsigned long) base << CLICK_SHIFT;
			m_geom[RAM_DEV].dv_size = bytesize;
			break;
		case MIOCSPSINFO:
			// MM of FS set the address of their process table
			if (m_ptr->PROC_NR == MM_PROC_NR){
				psinfo.mproc = (vir_bytes) m_ptr->ADDRESS;
			}else
			if (m_ptr->PROC_NR == FS_PROC_NR){
				psinfo.fproc = (vir_bytes) m_ptr->ADDRESS;
			}else{
				return (EPERM);
			}
			break;
		case MIOCGPINFO:
			// the ps program wants the process table addresses
			psinfo_phys = numap(m_ptr->PROC_NR,(vir_bytes) m_ptr->ADDRESS,sizeof(psinfo));
			if (psinfo_phys == 0) return (EFAULT);
			phys_copy(vir2phys(&psinfo),psinfo_phys,(phys_bytes) sizeof(psinfo));
			break;
		default:
			return (do_diocntl(&m_dtab,m_ptr));
	}
	return (OK);
}

/*======================================================================================*
 * 					m_geometry					*
 *======================================================================================*/
PRIVATE void m_geometry(entry)
struct partition *entry;
{
	// memory device don't have a geometry, but the outside world insists.
	entry->cylinders = div64u(m_geom[m_device].dv_size,SECTOR_SIZE) / (64*32);
	entry->heads = 64;
	entry->sectors = 32;
}
