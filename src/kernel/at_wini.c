// this file contains the device dependent part of a driver for the IBM-AT
// winchester controller.
// 
// the file contains one entry point:
//
// 	at_winchester_task:	main entry when system is brought up

#include "kernel.h"
#include "driver.h"
#include "drvlib.h"

#if ENABLE_AT_WINI

// I/O ports used by winchester disk controllers

// read and write registers
#define REG_BASE0		0x1F0	// base register of controller 0
#define REG_BASE1		0x170	// base register of controller 1
#define REG_DATA		0	// data register (offset from the base reg)
#define REG_PRECOMP		1	// start of write precompensation
#define REG_COUNT		2	// sectors to transfer
#define REG_SECTOR		3	// sector number
#define REG_CYL_LO		4	// low byte of cylinder number
#define REG_CYL_HI		5	// hight byte of cylinder number
#define REG_LDH			6	// lba,drive and head
#define LDH_DEFAULT		0xA0	// ECC enable,512 bytes per sector
#define LDH_LBA			0x40	// use LBA addressing
#define ldh_init(drive)		(LDH_DEFAULT | ((drive) << 4))

// read only registers
#define REG_STATUS		7	// status
#define STATUS_BSY		0x80	// controller busy
#define STATUS_RDY		0x40	// drive ready
#define STATUS_WF		0x20	// write fault
#define STATUS_SC		0x10	// seek complete(obsolete)
#define STATUS_DRQ		0x08	// data transfer request
#define STATUS_CRD		0x04	// corrected data
#define STATUS_IDX		0x02	// index pulse
#define STATUS_ERR		0x01	// error
#define REG_ERROR		1	// error code
#define ERROR_BB		0x80	// bad block
#define ERROR_ECC		0x40	// bad ecc bytes
#define ERROR_ID		0x10	// id not found
#define ERROR_AC		0x04	// aborted command
#define ERROR_TK		0x02	// track zero error
#define ERROR_DM		0x01	// no data address mark

// write only registers
#define REG_COMMAND		7	// command
#define CMD_IDLE		0x00	// for w_command:drive idle
#define CMD_RECALIBRATE		0x10	// recalibrate drive
#define CMD_READ		0x20	// read data
#define CMD_WRITE		0x30	// write data
#define CMD_READVERIFY		0x40	// read verify
#define CMD_FORMAT		0x50	// format track
#define CMD_SEEK		0x70	// seek cylinder
#define CMD_DIAG		0x90	// execute device diagnostics
#define CMD_SPECIFY		0x91	// specify parameters
#define ATA_IDENTIFY		0xEC	// identify drive
#define REG_CTL			0x206	// control register
#define CTL_NORETRY		0x80	// disable access retry
#define CTL_NOECC		0x40	// disable ecc retry
#define CTL_EIGHTHEADS		0x08	// more than eight heads
#define CTL_RESET		0x04	// reset controller
#define CTL_INTDISABLE		0x02	// disable interrupts

// interrupt request line
#define AT_IRQ0			14	// interrupt number of controller 0
#define AT_IRQ1			15	// interrupt number of controller 1

// general command block
struct command{
	u8_t	precomp;		// REG_PRECOM etc.
	u8_t	count;
	u8_t	sector;
	u8_t	cyl_lo;
	u8_t	cyl_hi;
	u8_t	ldh;
	u8_t	command;
};

// error code
#define ERR			(-1)	// general error
#define ERR_BAD_SECTOR		(-2)	// illegal mark block detection

// some controllers do not interrupt and the clock starts
#define WAKEUP			(32*HZ)	// drive can be disabled for up to 31 seconds

// etc.
#define MAX_DRIVES		4	// this driver supports 4 drives(hd0 - hd19)
#if _WORD_SIZE > 2
#define MAX_SECS		256	// the controller can transfer up to this number of sectors
#else
#define MAX_SECS		127	// but 16bit process does not transfer
#endif
#define MAX_ERRORS		4	// how often to try rd/wt before quit
#define NR_DEVICES		(MAX_DRIVES * DEV_PER_DRIVE)
#define SUB_PER_DRIVE		(NR_PARTITIONS * NR_PARTITIONS)
#define NR_SUBDEVS		(MAX_DRIVES * SUB_PER_DRIVE)
#define TIMEOUT			32000	// controller timeout (in ms)
#define RECOVERYTIME		500	// controller recovery time (in ms)
#define INITIALIZED		0x01	// initialize drive
#define DEAF			0x02	// controller must be reset
#define SMART			0x04	// drive supports ATA command

// variables
PRIVATE struct wini{			// main drive struct,one entry per drive
	unsigned state;			// drive state : deaf,initialized,dead
	unsigned base;			// base register of register file
	unsigned irq;			// interrupt required line
	unsigned lcylinders;		// logical number of cylinders(BIOS)
	unsigned lheads;		// logical number of heads
	unsigned lsectors;		// logical number of sectors per track
	unsigned pcylinders;		// physical number of cylinders(translated)
	unsigned pheads;		// physical number of heads
	unsigned psectors;		// physical number of sectors per track
	unsigned ldhpref;		// top four bytes of the LDH(head) register
	unsigned precomp;		// write precompensation cylinder / 4
	unsigned max_count;		// max request for this drive
	unsigned open_ct;		// in-use count
	struct device part[DEV_PER_DRIVE];	// primary partition: hd[0-4]
	struct device subpart[SUB_PER_DRIVE];	// subpartition: hd[1-4][a-d]
} wini[MAX_DRIVES], *w_wn;

PRIVATE struct trans{
	struct iorequest_s *iop;	// belong to this I/O request
	unsigned long block;		// first sector to transfer
	unsigned count;			// byte count
	phys_bytes phys;		// user physical address
} wtrans[NR_IOREQS];

PRIVATE struct trans *w_tp;		// add a transfer request
PRIVATE unsigned w_count;		// number of bytes to transfer
PRIVATE unsigned long w_nextblock;	// nextblock to transfer on disk
PRIVATE int w_opcode;			// DEV_READ or DEV_WRITE
PRIVATE int w_command;			// currently running command
PRIVATE int w_status;			// state after interrupt
PRIVATE int w_drive;			// selected drive
PRIVATE struct device *w_dv;		// base and size of device

FORWARD _PROTOTYPE(void init_params,(void));
FORWARD _PROTOTYPE( int w_do_open,(struct driver *dp,message *m_ptr));
FORWARD _PROTOTYPE( struct device *w_prepare,(int device));
FORWARD _PROTOTYPE( int w_identify,(void));
FORWARD _PROTOTYPE( char *w_name,(void));
FORWARD _PROTOTYPE( int w_specify,(void));
FORWARD _PROTOTYPE( int w_schedule,(int proc_nr,struct iorequest_s *iop));
FORWARD _PROTOTYPE( int w_finish,(void));
FORWARD _PROTOTYPE( int com_out,(struct command *cmd));
FORWARD _PROTOTYPE( void w_need_reset,(void));
FORWARD _PROTOTYPE( int w_do_close,(struct driver *dp,message *m_ptr));
FORWARD _PROTOTYPE( int com_simple,(struct command *cmd));
FORWARD _PROTOTYPE( void w_timeout,(void));
FORWARD _PROTOTYPE( int w_reset,(void));
FORWARD _PROTOTYPE( int w_intr_wait,(void));
FORWARD _PROTOTYPE( int w_waitfor,(int mask,int value));
FORWARD _PROTOTYPE( int w_handler,(int irq));
FORWARD _PROTOTYPE( void w_geometry,(struct partition *entry));

// w_waitfor loop once expanded for speed
#define waitfor(mask,value)		\
	((in_byte(w_wn->base + REG_STATUS) & mask) == value \
	 	|| w_waitfor(mask,value))

// entry point to this driver
PRIVATE struct driver w_dtab = {
	w_name,		// current device name
	w_do_open,	// initialize the device on open or mount request
	w_do_close,	// release the device
	do_diocntl,	// get or set the geometry of the partition
	w_prepare,	// prepare for I/O of a given minor device
	w_schedule,	// pre-calculate cylinders,heads,sectors,etc.
	w_finish,	// do I/O
	nop_cleanup,	// nothing to clean up
	w_geometry,	// tell the geometry of the disc
};

#if ENABLE_ATAPI
#include "atapi.c"	// spare code for ATAPI CD_ROM
#endif

/*======================================================================================*
 * 					at_winchester_task				*
 *======================================================================================*/
PUBLIC void at_winchester_task()
{
	// set special disk parameters then call the generic main loop.
	init_params();
	driver_task(&w_dtab);
}
/*======================================================================================*
 * 					init_params					*
 *======================================================================================*/
PRIVATE void init_params()
{
	// this routine is called at startup to initialize the drive parameters
	u16_t prav[2];
	unsigned int vector;
	int drive,nr_drives,i;
	struct wini *wn;
	u8_t params[16];
	phys_bytes param_phys = vir2phys(params);

	// get the number of drives from the BIOS data area
	phys_copy(0x475L,param_phys,1L);
	if ((nr_drives = param[0]) > 2) nr_drives = 2;

	for (drive=0;wn=wini;drive<MAX_DRIVES;drive++,wn++){
		if (drive < nr_drives){
			// copy the BIOS parameter vector
			vector = drive == 0 ? WINI_0_PARM_VEC : WINI_1_PARM_VEC;
			phys_copy(vector * 4L,vir2phys(parv),4L);

			// calculate the address of the parameters and copy them
			phys_copy(hclick_to_physb(parv[1]) + parv[0],param_phys,16L);

			// copy the parameters to the structures of the drive
			wn->lcylinders = bp_cylinders(params);
			wn->lheads = bp_heads(params);
			wn->lsectors = bp_sectors(params);
			wn->precomp = bp_precomp(params) >> 2;
		}
		wn->ldhpref = ldh_init(drive);
		wn->max_count = MAX_SECS << SECTOR_SHIFT;
		if (drive < 2){
			// cotroller 0
			wn->base = REG_BASE0;
			wn->irq = AT_IRQ0;
		}else{
			wn->base = REG_BASE1;
			wn->irq = AT_IRQ1;
		}
	}
}
/*======================================================================================*
 * 					w_do_open					*
 *======================================================================================*/
PRIVATE int w_do_open(dp,m_ptr)
struct drive *dp;
message *m_ptr;
{
	// device open : initialize the controller and read the partition table
	struct wini *wn;

	if (w_prepare(m_ptr->DEVICE) == NIL_DEV) return(ENXIO);
	wn = w_wn;

	if (wn->state == 0){
		// try to indentify the device
		if (w_identify() != OK){
			printf("%s: probe failed\n",w_name());
			if (wn->state & DEAF) w_reset();
			wn->state = 0;
			return(ENXIO);
		}
	}
	if (wn->open_ct++ == 0){
		// split disk
		partition(&w_dtab,w_drive * DEV_PER_DRIVE,P_PRIMARY);
	}
	return(OK);
}

/*======================================================================================*
 * 					w_prepare					*
 *======================================================================================*/
PRIVATE struct device *w_prepare(device)
int device;
{
	// prepare for I/O on a device
	
	// nothing to transfer yet
	w_count = 0;

	if (device < NR_DEVICES){			// hd0,hd1,....
		w_drive = device / DEV_PER_DEVICE;	// save drive number
		w_wn = &wini[w_drive];
		w_dv = &w_wn->part[device % DEV_PER_DRIVE];
	}else
	if ((unsigned) (device-=MMINOR_hd1a) < NR_SUBDEVS) { // hd1a,hd1b
		w_drive = device / SUB_PER_DRIVE;
		w_wn = &wini[w_drive];
		w_dv = &w_wn->subpart[device % SUB_PER_DRIVE];
	}else{
		return(NIL_DEV);
	}
	return (w_dv);
}

/*======================================================================================*
 * 					w_identify					*
 *======================================================================================*/
PRIVATE int w_identify()
{
	// find out if a device exists,if it is an old AT disk,
	// or a newer ATA drive,a removable media device,etc.
	struct wini *wn = w_wn;
	struct command cmd;
	char id_string[40];
	int i,r;
	unsigned long size;
#define id_byte(n)		(&tmp_buf[2 * (n)])
#define id_word(n)		(((u16_t) id_byte(n)[0] << 0) \
				| ((u16_t) id_byte(n)[1] << 8))
#define id_longword(n)		(((u32_t) id_byte(n)[0] << 0) \
				| ((u32_t) id_byte(n)[1] << 8) \
				| ((u32_t) id_byte(n)[2] << 16) \
				| ((u32_t) id_byte(n)[3] << 24))

	// check if the register exists
	r = in_byte(wn->base + REG_CYL_LO);
	out_byte(wn->base + REG_CYL_LO,~r);
	if (in_byte(wn->base + REG_CYL_LO) == r) return (ERR);

	// looks like OK ; register IRQ and try ATA identification command
	put_irq_handler(wn->irq,w_handler);
	enable_irq(wn->irq);

	cmd.ldh = wn->ldhpref;
	cmd.command = ATA_IDENTIFY;
	if (com_simple(&cmd) == OK){
		// this is ATA device
		wn->state |= SMART;

		// device info
		port_read(wn->base + REG_DATA,tmp_phys,SECTOR_SIZE);

		// why string bytes are swapped
		for (i=0;i<40;i++) id_string[i] = id_byte(27)[i^1];

		// CHS conversion mode is preferred
		wn->pcylinders = id_word(1);
		wn->pheads = id_word(3);
		wn->psectors = id_word(6);
		size = (u32_t) wn->pcylinders * wn->pheads * wn->psectors;

		if ((id_byte(49)[1] & 0x02) && size > 512L*1024*2){
			// the drive can be LBA, so it's big enough that you can
			// expect to mistakes.
			wn->ldhpref |= LDH_LBA;
			size = id_longword(60);
		}

		if (wn->lcylinders == 0){
			// no BIOS paraametes ? in that case it will succeed
			wn->lcylinders = wn->pcylinders;
			wn->lheads = wn->pheads;
			wn->lsectors = wn->psectors;
			while(wn->lcylinders > 1024){
				wn->lheads *= 2;
				wn->lcylinders /= 2;
			}
		}
	}else{
		// not an ATA device; no conversion,no special features.
		// don't touh it unless the BIOS knows about it.
		if (wn->lcylinders == 0) return (ERR);			// no BIOS parameter
		wn->pcylinders = wn->lcylinders;
		wn->pheads = wn->lheads;
		wn->psectors = wn->lsectors;
		size = (u32_t) wn->pcylinders * wn->pheads * wn->psectors;
	}
	// end with 4GB
	if (size > ((u32_t) -1) / SECTOR_SIZE) size = ((u32_t) -1)/ SECTOR_SIZE;

	// overall drive base and size
	wn->part[0].dv_base = 0;
	wn->part[0].dv_size = size << SECTOR_SHIFT;

	if (w_specify() != OK && w_specify() != OK) return (ERR);

	printf("%s: ",w_name());
	if (wn->state & SMART){
		printf("%.40s\n",id_string);
	}else{
		printf("%ux%ux%u\n",wn->pcylinders,wn->pheads,wn->psectors);
	}
	return (OK);
}
/*======================================================================================*
 * 					w_name						*
 *======================================================================================*/
PRIVATE char *w_name()
{
	// return the name for the current device
	static char name[] = "at-hd15";
	unsigned device = w_drive * DEV_PER_DRIVE;

	if (device < 10){
		name[5] = '0' + device;
		name[6] = '0';
	}else{
		name[5] = '0' + device / 10;
		name[6] = '0' + device % 10;
	}
	return name;
}
/*======================================================================================*
 * 					w_specify					*
 *======================================================================================*/
PRIVATE int w_specify()
{
	// routine to initialize the drive after boot or when a reset is needed
	struct wini *wn = w_wn;
	struct command cmd;

	if ((wn->state & DEAF) && w_reset() != OK) return(ERR);

	// specify parameters: precompensation,number of heads and sectors
	cmd.precomp = wn->precomp;
	cmd.count = wn->psectors;
	cmd.ldh = w_wn->ldhpref | (wn->pheads - 1);
	cmd.command = CMD_SPECIFY;		// specify some parameters
	
	if (com_simple(&cmd)!=OK) return(ERR);

	if (!(wn->state & SMART)){
		// calibrate an old disk
		cmd.sector = 0;
		cmd.cyl_lo = 0;
		cmd.cyl_hi = 0;
		cmd.ldh = w_wn->ldhpref;
		cmd.command = CMD_RECALIBRATE;

		if (com_simple(&cmd) != OK) return(ERR);
	}

	wn->state != INITIALIZED;
	return(OK);
}


/*======================================================================================*
 * 					w_schedule					*
 *======================================================================================*/
PRIVATE int w_schedule(proc_nr,iop)
int proc_nr;				// process doint the request
struct iorequest_s *iop;		// read or write request pointer
{
	// collect I/O requests for consecutive blocks so that they can be read
	// / written with a single controller command (with enough time to calculate
	// the next consecutive request while the unwanted blocks pass).
	struct wini *wn = w_wn;
	int r,opcode;
	unsigned long pos;
	unsigned nbytes,count;
	unsigned long block;
	phys_bytes user_phys;

	// number of bytes for this read/write
	nbytes = iop->io_nbytes;
	if ((nbytes & SECTOR_MASK) != 0) return(iop->io_nbytes=EINVAL);
	// from this position on the device or to this position
	pos = iop->io_position;
	if ((pos & SECTOR_MASK) != 0) return(iop->io_nbytes = EINVAL);

	// to or from this user address
	user_phys = numap(proc_nr,(vir_bytes) iop->io_buf,nbytes);
	if (user_phys == 0) return (iop->io_nbytes = EINVAL);
	// read or write ?
	opcode = iop->io_request & ~OPTIONAL_IO;

	// which block on disk and how close it is to EOF?
	if (pos >= w_dv->dv_size) return(OK);			// if EOF
	if (pos + nbytes > w_dv->dv_size) nbytes = w_dv->dv_size - pos;
	block = (w_dv->dv_base + pos) >> SECTOR_SHIFT;

	if (w_count > 0 && block != w_nextblock){
		// this new request cannot be combined into a job under construction
		if ((r=w_finish())!=OK) return(r);
	}
	// next consecutive block
	w_nextblock = block + (nbytes >> SECTOR_SHIFT);	
	// while the request has "unscheduled" bytes:
	do{
		count = nbytes;
		if (w_count == wn->max_count){
			// drive cannot run more than max_count at a time
			if ((r = w_finish()) != OK) return(r);
		}
		if (w_count + count > wn->max_count)
			count = wn->max_count - w_count;
		if (w_count == 0){
			// first request in column,initialization
			w_opcode = opcode;
			w_tp = wtrans;
		}

		// save I/O parameter
		w_tp->iop = iop;
		w_tp->block = block;
		w_tp->count = count;
		w_tp->phys = user_phys;

		// update count
		w_tp++;
		w_count += count;
		block += count >> SECTOR_SHIFT;
		user_phys += count;
		nbytes -= count;
	} while(nbytes > 0);

	return (OK);
}

/*======================================================================================*
 * 					w_finish					*
 *======================================================================================*/
PRIVATE int w_finish()
{
	// execute the I/O requests collected in wtrans[]
	struct trans *tp = wtrans;
	struct wini *wn = w_wn;
	int r,errors;
	struct command cmd;
	unsigned cylinder,head,sector,secspcyl;
	if (w_count==0) return (OK);		// pseudo end
	
	r = ERR;				// trigger the first com_out
	errors = 0;

	do{
		if (r != OK){
			// controller must be (re) programmed

			// first,do a check to see if reinitialized is needed
			if (!(wn->state & INITIALIZED) && w_specify() != OK)
				return (tp->iop->io_nbytes = EIO);

			// tell the controller to transfer w_count bytes
			cmd.precomp = wn->precomp;
			comd.count = (w_count >> SECTOR_SHIFT) & BYTE;
			if (wn->ldhpre & LDH_LBA){
				cmd.sector = (tp->block >> 0) & 0xFF;
				cmd.cyl_lo = (tp->block >> 8) & 0xFF;
				cmd.cyl_hi = (tp->block >> 16) & 0xFF;
				cmd.ldh = wn->ldhpref | ((tp->block >> 24) & 0xF);
			}else{
				secspcyl = wn->pheads * wn->psectors;
				cylinder = tp->block / secspcyl;
				head = (tp->block % secspcyl) / wn->psectors;
				sector = tp->block % wn->psectors;
				cmd.sector = sector + 1;
				cmd.cyl_lo = cylinder & BYTE;
				cmd.cyl_hi = (cylinder >> 8) & BYTE;
				cmd.ldh = wn->ldhpref | head;
			}
			cmd.command = w_opcode == DEV_WRITE ? CMD_WRITE : CMD_READ;
			if ((r = com_out(&cmd)) != OK){
				if (++errors == MAX_ERRORS){
					w_command = CMD_IDLE;
					return (tp->iop->io_nbytes = EIO);
				}
				continue;	// retry
			}
		}
		// for each sector, wait for interrupt to fetch (read) data,
		// or supply data to the controller and wait for an interrupt(write)
		if (w_opcode == DEV_READ){
			if ((r = w_intr_wait()) == OK){
				// copy data from device buffer to user space
				port_read(wn->base + REG_DATA, rp->phys,SECTOR_SIZE);
				tp->phys += SECTOR_SIZE;
				tp->iop->io_nbytes -= SECTOR_SIZE;
				w_count -= SECTOR_SIZE;
				if ((tp->count -= SECTOR_SIZE ) == 0) tp++;
			}else{
				// illegal data?
				if (w_status & STATUS_DRQ){
					port_read(wn->base + REG_DATA,tmp_phys,SECTOR_SIZE);
				}
			}
		}else{
			// wait for data request
			if (!waitfor(STATUS_DRQ,STATUS_DRQ)){
				r = ERR;
			} else{
				// fill the drive buffer
				port_write(wn->base + REG_DATA,tp->phys,SECTOR_SIZE);
				r = w_intr_wait();
			}
			
			if (r == OK){
				// register a successfully write byte
				tp->phys += SECTOR_SIZE;
				tp->iop->io_nbytes -= SECTOR_SIZE;
				w_count -= SECTOR_SIZE;
				if ((tp->count -= SECTOR_SIZE) == 0) tp++;
			}
		}
		if (r != OK){
			// if the sector is marked invalid or there are too many
			// errors,do not retry
			if (r == ERR_BAD_SECTOR || ++errors == MAX_ERRORS){
				w_command = CMD_IDLE;
				return (tp->iop->io_nbytes = EIO);
			}

			// return if in the middle ,but stop if option I/O
			if (errors == MAX_ERRORS / 2){
				w_need_reset();
				if (tp->iop->io_request & OPTIONAL_IO){
					w_command = CMD_IDLE;
					return (tp->iop->io_nbytes = EIO):
				}
			}
			continue;				// retry
		}
		errors = 0;
	}while(w_count > 0);

	w_command = CMD_IDLE;
	return (OK);
}
/*======================================================================================*
 * 					com_out						*
 *======================================================================================*/
PRIVATE int com_out(cmd)
struct command *cmd;				// command block
{
	// output the command block to the winchester controller and return the status
	struct wini *wn = w_wn;
	unsigned base = wn->base;

	if (!waitfor(STATUS_BSY,0)){
		printf("%s: controller not ready\n",w_name());
		return (ERR);
	}

	// select drive
	out_byte(base + REG_LDH,cmd->ldh);

	if (!waitfor(STATUS_BDY,0)){
		printf("%s: drive not ready\n",w_name());
		return (ERR);
	}

	// schedule a wakeup call.some controllers don't work.
	clock_mess(WAKEUP,w_timeout);

	out_byte(base + REG_CTL,wn->pheads >= 8 ? CTL_EIGHTHEADS : 0);
	out_byte(base + REG_PRECOMP,cmd->precomp);
	out_byte(base + REG_COUNT,cmd->count);
	out_byte(base + REG_SECTOR,cmd->sector);
	out_byte(base + REG_CYL_LO,cmd->cyl_lo);
	out_byte(base + REG_CYL_HI,cmd->cyl_hi);
	lock();
	out_byte(base + REG_COMMAND,cmd->command);
	w_command = cmd->command;
	w_status = STATUS_BSY;
	unlock();
	return(OK);
}
/*======================================================================================*
 * 					w_need_rest					*
 *======================================================================================*/
PRIVATE void w_need_reset()
{
	// need to reset controller
	struct wini *wn;
	for (wn = wini; wn < &wini[MAX_DRIVES];wn++){
		wn->state |= DEAF;
		wn->state &= ~INITIALIZED;
	}
}
/*======================================================================================*
 * 					w_do_close					*
 *======================================================================================*/
PRIVATE int w_do_close(dp,m_ptr)
struct driver *dp;
message *m_ptr;
{
	// close device : release device
	if (w_prepare(m_ptr->DEVICE) == NIL_DEV) return(ENXIO);
	w_wn->open_ct--;
	return(OK);
}		
/*======================================================================================*
 * 					com_simple					*
 *======================================================================================*/
PRIVATE int com_simple(cmd)
struct command *cmd;				// command block
{
	// simple controller command, only one interrupt,no data out phase
	int r;

	if ((r = com_out(cmd)) == OK) r = w_intr_wait();
	w_command = CMD_IDLE;
	return (r);
}
/*======================================================================================*
 * 					w_timeout					*
 *======================================================================================*/
PRIVATE void w_timeout()
{
	struct wini *wn = w_wn;

	switch(w_command){
		case CMD_IDLE:
			break;			// greate
		case CMD_READ:
		case CMD_WRITE:
			// only possible on PC:controller dones not respond

			// multi-sector I/O restructuins seem to be in effect
			if (wn->max_count > 8 * SECTOR_SIZE){
				wn->max_count = 8 * SECTOR_SIZE;
			}else{
				wn->max_count = SECTOR_SIZE;
			}

			//  procced as it is
		default:
			// another command
			printf("%s: timeout on command %02x\n",w_name(),w_command);
			w_need_reset();
			w_status = 0;
			interrupt(WINCHESTER);
	}
}

/*======================================================================================*
 * 					w_reset						*
 *======================================================================================*/
PRIVATE int w_reset()
{
	// issue a reset to the controller. this happens after a failure such as the
	// controller rejecting the response.
	struct wini *wn;
	int err;

	// wait for internal drive recover
	milli_delay(RECOVERYTIME);

	// strobe reset bit
	out_byte(w_wn->base + REG_CTL,CTL_RESET);
	milli_delay(1);
	out_byte(w_wn->base + REG_CTL,0);
	milli_delay(1);

	// wait for the controller to be ready
	if (!w_waitfor(STATUS_BSY | STATUS_RDY,STATUS_RDY)){
		printf("%s: reset failed,drive busy\n",w_name());
		return (ERR);
	}

	// you have to check the error register here,but some drives confuse it
	for (wn = wini;wn<&wini[MAX_DRIVES];wn++){
		if (wn->base == w_wn->base) wn->state &= ~DEAF;
	}
	return (OK);
}

/*======================================================================================*
 * 					w_intr_wait					*
 *======================================================================================*/
PRIVATE int w_intr_wait()
{
	// waits for task completion interrupt and retuns result
	message mess;
	int r;

	// wait for an interurpt that sets w_status to "not busy".
	while(w_status & STATUS_BSY) receive(HARDWARE,&mess);

	// check status
	lock();
	if ((w_status & (STATUS_BSY | STATUS_RDY | STATUS_WF | STATUS_ERR)) == STATUS_RDY){
		r = OK;
		w_status |= STATUS_BSY;			// suppose I/O is still in use
	}else
	if ((w_status & STATUS_ERR) && (in_byte(w_wn->base + REG_ERROR) & ERROR_BB)){
		r = ERR_BAD_SECTOR;			// sectors marked as invalid,retries
							// have no effect
	}else{
		r = ERR;				// some other error
	}
	unlock();
	return (r);
}
/*======================================================================================*
 * 					w_waitfor					*
 *======================================================================================*/
PRIVATE int w_waitfor(mask,value)
int mask;						// status mask
int value;						// required status
{
	// wait for the controller to reach the required status.
	// return zero on timeout
	struct milli_state ms;
	milli_start(&ms);
	do{
		if ((in_byte(w_wn->base + REG_STATUS) & mask) == value) return 1;
	}while(milli_elapsed(&ms) < TIMEOUT);

	w_need_reset();					// controller be deaf status
	return (0);
}
/*======================================================================================*
 * 					w_handler					*
 *======================================================================================*/
PRIVATE int w_handler(irq)
int irq;
{
	// disk interrupt,send a message to the winchester task and re-enable interrupt
	w_status = in_byte(w_wn->base + REG_STATUS);	// accept interrupt
	interrupt(WINCHESTER);
	return 1;
}

/*======================================================================================*
 * 					w_geometry					*
 *======================================================================================*/
PRIVATE void w_geometry(entry)
struct partition *entry;
{

	entry->cylinders = w_wn->lcylinders;
	entry->heads = w_wn->lheads;
	entry->sectors = w_wn->lsectors;
}
#endif // ENABLE_AT_WINI
