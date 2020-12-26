


struct super_block;

#include "fs.h"
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/boot.h>
#include "buf.h"
#include "dev.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "param.h"
#include "super.h"

FORWARD _PROTOTYPE( void buf_pool,(void));
FORWARD _PROTOTYPE( void fs_init,(void));
FORWARD _PROTOTYPE( void get_boot_parameters,(void));
FORWARD _PROTOTYPE( void get_work,(void));
FORWARD _PROTOTYPE( void load_ram,(void));
FORWARD _PROTOTYPE( void load_super,(Dev_t super_dev));

/*======================================================*
 *			main				*
 *======================================================*/
PUBLIC void main()
{
	int error;
	fs_init();
	while(TRUE){
		get_work();
		
		fp = &fproc[who];
		super_user = (fp->fp_effuid == SU_UID ? TRUE : FALSE);
		dont_reply = FALSE;

		if (fs_call < 0 || fs_call >= NCALLS)
			error = EBADCALL;
		else
			error = (*call_vector[fs_call])();
		if (dont_reply) continue;
		reply(who,error);
		if (rdahed_inode != NIL_INODE) read_ahead();
	}
}

/*======================================================*
 *			get_work			*
 *======================================================*/
PRIVATE void get_work()
{
	register struct fproc *rp;

	if (reviving != 0){
		for (rp=&fproc[0];rp<&fproc[NR_PROC];rp++)
			if (rp->fp_revived == REVIVING){
				who = (int) (rp - fproc);
				fs_call = rp->fp_fd & BYTE;
				fd = (rp->fp_fd >> 8) & BYTE;
				buffer = rp->fp_buffer;
				nbytes = rp->fp_nbytes;
				rp->fp_suspended = NOT_SUSPENDED;
				rp->fp_revived = NOT_REVIVING;
				reviving--;
				return;
			}
		panic("get_work couldn't revive anyone",NO_NUM);
	}

	if (receive(ANY,&m) != OK) panic("fs receive error",NO_NUM);

	who = m.m_source;
	fs_call = m.m_type;
}

/*======================================================*
 *			reply				*
 *======================================================*/
PUBLIC void reply(whom,result)
int whom;
int result;
{
	reply_type = result;
	send(whom,&m1);
}

/*======================================================*
 *			fs_init				*
 *======================================================*/
PRIVATE void fs_init()
{
	register inode *rip;
	int i;
	message mess;

	fp = (struct fproc *) NULL;
	who = FS_PROC_NR;

	buf_pool();
	get_boot_parameters();
	load_ram();

	load_super(ROOT_DEV);
	
	for (i=0;i<=LOW_USER;i+=1){
		if (i==FS_PROC_NR) continue;
		fp = &fproc[i];
		rip = get_inode(ROOT_DEV,ROOT_INODE);
		fp->fp_rootdir = rip;
		dup_inode(rip);
		fp->fp_workdir = rip;
		fp->fp_realuid = (uid_t) SYS_UID;
		fp->fp_effuid = (uid_t) SYS_UID;
		fp->fp_realgid = (gid_t) SYS_GID;
		fp->fp_effgid = (gid_t) SYS_GID;
		fp->fp_umask = ~0;
	}

	if (SUPER_SIZE > BLOCK_SIZE) panic("SUPER_SIZE > BLOCK_SIZE",NO_NUM);
	if (BLOCK_SIZE % V2_INODE_SIZE != 0)
		panic("BLOCK_SIZE % V2_INODE_SIZE != 0",NO_NUM);
	if (OPEN_MAX > 127) panic("OPEN_MAX > 127",NO_NUM);
	if (NR_BUFS < 6) panic("NR_BUFS < 6",NO_NUM);
	if (V1_INODE_SIZE != 32) panic("V1 inode size != 32",NO_NUM);
	if (V2_INODE_SIZE != 64) panic("V2 inode size != 64",NO_NUM);
	if (OPEN_MAX > 8 * sizeof(long)) panic("Too few bits in fp_cloexec",NO_NUM);

	mess.m_type = DEV_IOCTL;
	mess.PROC_NR = FS_PROC_NR;
	mess.REQUEST = MIOSPSINFO;
	mess.ADDRESS = (void *) fproc;
	(void) sendrec(MEM,&mess);
}

/*======================================================*
 *			buf_pool			*
 *======================================================*/
PRIVATE void buf_pool()
{
	register struct buf *bp;

	bufs_in_use = 0;
	front = &buf[0];
	rear = &buf[NR_BUFS - 1];

	for (bp=&buf[0];bp<&buf[NR_BUFS];bp++){
		bp->b_blocknr = NO_BLOCK;
		bp->b_dev = NO_DEV;
		bp->b_next = bp + 1;
		bp->b_prev = bp - 1;
	}
	buf[0].b_prev = NIL_BUF;
	buf[NR_BUFS - 1].b_next = NIL_BUF;
	for (bp=&buf[0];bp<&buf[NR_BUFS];bp++) bp->b_hash = bp->b_next;
	buf_hash[0] = front;
}

/*======================================================*
 *			get_boot_parameters		*
 *======================================================*/
PUBLIC struct bparam_s boot_parameters;

PRIVATE void get_boot_parameters()
{
	m1.m_type = SYS_GBOOT;
	m1.PROC1 = FS_PROC_NR;
	m1.MEM_PTR = (char *) &boot_parameters;
	(void) sendrec(SYSTASK,&m1);
}

/*======================================================*
 *			load_ram			*
 *======================================================*/
PRIVATE void load_ram()
{
	register struct buf *bp,*bp1;
	long k_loaded,lcount;
	u32_t ram_size,fsmax;
	zone_t zones;
	struct super_block *sp,*dsp;
	block_t b;
	int major,task;
	message dev_mess;

	ram_size = boot_parameters.bp_ramsize;
	major = (ROOT_DEV >> MAJOR) & BYTE;
	task = dmap[major].dmap_task;
	dev_mess.m_type = DEV_OPEN;
	dev_mess.DEVICE = ROOT_DEV;
	dev_mess.COUNT = R_BIT|W_BIT;
	(*dmap[major].dmap_open)(task,&dev_mess);
	if (dev_mess.REP_STATUS != OK) panic("Cannot open root device",NO_NUM);
	if (ROOT_DEV == DEV_RAM){
		major = (IMAGE_DEV >> MAJOR) & BYTE;
		task = dmap[major].dmap_task;
		dev_mess.m_type = DEV_OPEN;
		dev_mess.DEVICE = IMAGE_DEV;
		dev_mess.COUNT = R_BIT;
		(*dmap[major].dmap_open)(task,&dev_mess);
		if (dev_mess.REP_STATUS != OK) panic("Cannot open root device",NO_NUM);

		sp = &super_block[0];
		sp->s_dev = IMAGE_DEV;
		if (read_super(sp) != OK) panic("Bad root file system",NO_NUM);
		
		lcount = sp->s_zones << sp->s_log_zone_size;

		if (ram_size < lcount) ram_size = lcount;
		fsmax = (u32_t) sp->s_zmap_blocks * CHAR_BIT * BLOCK_SIZE;
		fsmax = (fsmax + (sp->s_firstdatazone-1)) << sp->s_log_zone_size;
		if (ram_size > fsmax) ram_size = fsmax;
	}

	m1.m_type = DEV_IOCTL;
	m1.PROC_NR = FS_PROC_NR;
	m1.REQUEST = MIOCRAMSIZE;
	m1.POSITION = ram_size;
	if (sendrec(MEM,&m1) != OK)
		panic("FS can't sync up with MM",NO_NUM);
	if (ROOT_DEV != DEV_RAM) return;

	printf("Loading RAM disk.\33[23CLoaded:	OK");

	inode[0].i_mode = I_BLOCK_SPECIAL;
	inode[0].i_size = LONG_MAX;
	inode[0].i_dev = IMAGE_DEV;
	inode[0].i_zone[0] = IMAGE_DEV;

	for (b=0;b<(block_t) lcount;b++){
		bp = rahead(&inode[0],b,(off_t) BLOCK_SIZE * b,BLOCK_SIZE);
		bp1 = get_block(ROOT_DEV,b,NO_READ);
		memcpy(bp1->b_data,bp->b_data,(size_t) BLOCK_SIZE);
		bp1->b_dirt = DIRTY;
		put_block(bp,FULL_DATA_BLOCK);
		put_block(bp1,FULL_DATA_BLOCK);
		k_loaded = ((long) b * BLOCK_SIZE)/1024L;
		if (k_loaded % 5 == 0) printf("\b\b\b\b\b\b\b%5ldK",k_loaded);
	}
	printf("\rRAM disk loaded.\33[K\n\n");

	dev_mess.m_type = DEV_CLOSE;
	dev_mess.DEVICE = IMAGE_DEV;
	(*dmap[major].dmap_close)(task,&dev_mess);
	invalidate(IMAGE_DEV);

	bp = get_block(ROOT_DEV,SUPER_BLOCK,NORMAL);
	dsp = (struct super_block *) bp->b_data;
	zones = ram_size >> sp->s_log_zone_size;
	dsp->s_nzones = conv2(sp->s_native,(u16_t) zones);
	dsp->s_zones = conv4(sp->s_native,zones);
	bp->b_dirt = DIRTY;
	put_block(bp,ZUPER_BLOCK);
}

/*======================================================*
 *			load_super			*
 *======================================================*/
PRIVATE void load_super(super_dev)
dev_t super_dev;
{
	int bad;
	register struct super_block *sp;
	register struct inode *rip;

	for (sp=&super_block[0];sp<&super_block[NR_SUPERS];sp++)
		sp->s_dev = NO_DEV;

	sp = &super_block[0];
	sp->s_dev = super_dev;

	bad = (read_super(sp) != OK);
	if (!bad){
		rip = get_inode(super_dev,ROOT_INODE);
		if ((rip->i_mode & I_TYPE) != I_DIRECTORY || rip->i_nlinks < 3) bad++;
	}
	if (bad) panic("Invalid root file system. Possibly wrong diskette.",NO_NUM);

	sp->s_imount = rip;
	dup_inode(rip);
	sp->s_isup = rip;
	sp->s_rd_only = 0;
	return;
}
