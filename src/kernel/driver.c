// this file contains device independent device driver interface.
//
//
//
//  the drivers support the following operations ( using message format m2 ):
//
//       m_type 	device		PROC_NR		count		position	address
//  ----------------------------------------------------------------------------------------------
//  |	DEV_OPEN  | 	device	  |	proc nr	   |		   |		    | 		 |
//  |-------------+---------------+----------------+---------------+----------------+------------+
//  |	DEV_CLOSE |	device	  |	proc nr	   |		   |		    |            |
//  |-------------+---------------+----------------+---------------+----------------+------------+
//  |	DEV_READ  |	device	  |	proc nr	   |	  bytes	   |	  offset    |	buf ptr	 |
//  |-------------+---------------+----------------+---------------+----------------+------------+
//  |	DEV_WRITE |	device	  |	proc nr	   |	  bytes	   |	  offset    |	buf ptr	 |
//  |-------------+---------------+----------------+---------------+----------------+------------+
//  | SCATTERED_IO|	device	  |	proc nr	   |	request    |	  	    |	iov ptr	 |
//  |-------------+---------------+----------------+---------------+----------------+------------+
//  |	DEV_IOCTL |	device	  |	proc nr    |	func code  |		    |	buf ptr	 |
//  |---------------------------------------------------------------------------------------------
//
//
//	the file contains one entry point :
//	
//		driver_task : 	called by the device dependent task entry
//
//

#include "kernel.h"
#include <sys/ioc_disk.h>
#include "driver.h"

#define BUF_EXTRA	0

// space required for variables
PRIVATE u8_t 	buffer[(unsigned) 2 * DMA_BUF_SIZE + BUF_EXTRA];
u8_t		*tmp_buf;					// can be a DMA buffer
phys_bytes 	tmp_phys;					// physical address of DMA buffer

FORWARD _PROTOTYPE( void init_buffer, (void));

/*======================================================================================*
 * 					driver_task					*
 *======================================================================================*/
PUBLIC void driver_task(dp)
struct driver *dp;					// device dependent entry point
{
	// main program for any device driver task
	int r,caller,proc_nr;
	message mess;

	init_buffer();					// get DMA buffer

	// here is the main loop of the disk task.it waits for a message, carries it
	// out , and sends a reply.
	while (TRUE){
		// first,wait for a disk block read or write request
		receive(ANY,&mess);
		caller = mess.m_source;
		proc_nr = mess.PROC_NR;

		switch(caller){
		case HARDWARE;
			// postpone interrupt
			continue;
		case FS_PROC_NR;
			// only legitimate caller
			break;
		default:
			printf("%s: got message from %d\n", (*dp->dr_name)(), caller);
			continue;
		}

		// stop working here
		switch(mess.m_type){
			case DEV_OPEN:	r = (*dp->dr_open)(dp,&mess);	break;
			case DEV_CLOSE:	r = (*dp->dr_close)(dp,&mess);	break;
			case DEV_IOCTL: r = (*dp->dr_ioctl)(dp,&mess);	break;
			case DEV_READ:
			case DEV_WRITE:	r = do_rdwt(dp,&mess);		break;
			case SCATTERED_IO:	r = do_vrdwt(dp,&mess);	break;
			default:	r = EINVAL;			break;
		}
		
		// clean up the postponed state
		(*dp->dr_cleanup)();

		// finally,compose and send a reply message
		mess.m_type = TASK_REPLY;
		mess.REP_PROC_NR = proc_nr;

		mess.REP_STATUS = r;		// number of bytes transferred or error code
		send(caller,&mess);		// send an answer to the caller
	}
}

/*======================================================================================*
 * 					init_buffer					*
 *======================================================================================*/
PRIVATE void init_buffer()
{
	// select a buffer that can safely be used for dmatransfers.
	// it may also be used to read partition tables and such.
	// its absolute address is 'tmp_phys',the normal address is 'tmp_buf'.
	
	tmp_buf = buffer;
	tmp_phys = vir2phys(buffer);

	if (tmp_phys == 0) panic("no DMA buffer",NO_NUM);

	if (dma_bytes_left(tmp_phys) < DMA_BUF_SIZE){
		// first half of buffer crosses a 64K boundary,can't DMA into that
		tmp_buf += DMA_BUF_SIZE;
		tmp_phys += DMA_BUF_SIZE;
	}
}
/*======================================================================================*
 * 					do_rdwt 					*
 *======================================================================================*/
PUBLIC int do_rdwt(dp,m_ptr)
struct driver *dp;			// device dependent entry point
message *m_ptr;				// pointer to read or write message
{
	// carry out a single read or write request
	struct iorequest_s	ioreq;
	int r;

	if (m_ptr->COUNT <= 0) return (EINVAL);

	if ((*dp->dr_prepare)(m_ptr->DEVICE) == NIL_DEV) return (ENXIO);

	ioreq.io_request = m_ptr->m_type;
	ioreq.io_buf = m_ptr->ADDRESS;
	ioreq.io_position = m_ptr->POSITION;
	ioreq.io_nbytes = m_ptr->COUNT;

	r = (*dp->dr_schedule)(m_ptr->PROC_NR,&ioreq);

	if (r==OK) (void)(*dp->dr_finish)();

	r = ioreq.io_nbytes;
	return (r<0 ? r : m_ptr->COUNT - r);
}
