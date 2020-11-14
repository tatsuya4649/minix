// IBM device driver utility functions.
//
// entry point:
// 	partition : partition a disk to the partition table(s) on it.

#include "kernel.h"
#include "driver.h"
#include "drvlib.h"

FORWARD _PROTOTYPE( void extpartition,(struct driver *dp,int extdv,unsigned long extbase));
FORWARD _PROTOTYPE( int get_part_table,(struct driver *dp,int device,unsigned long offset,
			struct part_entry *table));
FORWARD _PROTOTYPE( void sort,(struct part_entry *table));

/*======================================================================================*
 * 					partition					*
 *======================================================================================*/
PUBLIC void partition(dp,device,style)
struct device *dp;				// device dependent entry point
int device;					// split device
int style;					// partition style : floppy,primary,sub
{
	// this routine is called on first open to initialize the partition tables
	// of a device. it makes sure that each partition falls safely within
	// the device's limits. depending on the partition style we are either making
	// floppy partitions,primary partitions or subpartitions. only primary 
	// partitions are sorted,because they are shared with other operating systems
	// that expect this.
	struct part_entry table[NR_PARTITION], *pe;
	int disk,par;
	struct device *dv;
	unsigned long base,limit,part_limit;

	// get the geomtry of the device to partition
	if ((dv=(*dp->dr_prepare)(device)) == NIL_DEV || dv->dv_size  == 0) return;
	base = dv->dv_base >> SECTOR_SHIFT;
	limit = base + (dv->dv_size >> SECTOR_SHIFT);

	// read parition table of device
	if (!get_part_table(dp,device,0L,table)) return;

	// compute the device number of the first partition.
	switch (style){
		case P_FLOPPY:
			device +=  MINOR_fd0a;
			break;
		case P_PRIMARY:
			sort(table);		// sort primary partition table
			device += 1;
			break;
		case P_SUB:
			disk = device / DEV_PER_DRIVE;
			par = device % DEV_PER_DRIVE - 1;
			device = MINOR_hd1a + (disk * NR_PARTITIONS + par) * NR_PARTITIONS;

	}

	// find an array of devices
	if ((dv = (*dp->dr_prepare)(device)) == NIL_DEV) return;

	// set the geometry of the partitions from the partition table
	for (par=0;par<NR_PARTITIONS;par++,dv++){
		// shrink the partition to fit within the device
		pe = &table[par];
		part_limit = pe->lowsec + pe->size;
		if (part_limit < pe->lowsec) part_limit = limit;
		if (part_limit > limit) part_limit = limit;
		if (pe->lowsec < base) pe->lowsec = base;
		if (part_limit < pe->lowsec) part_limit = pe->lowsec;

		dv->dv_base = pe->lowsec << SECTOR_SHIFT;
		dv->dv_size = (part_limit - pe->lowsec) << SECTOR_SHIFT;

		if (style == P_PRIMARY){
			// each minix primary partition can be subpartitioned
			if (pe->sysind == MINIX_PART)
				partition(dp,device + par,P_SUB);

			// an extended partition has logical partitons
			if (pe->sysind == EXT_PART)
				extpartition(dp,device+par,pe->lowsec);
		}
	}
}
/*======================================================================================*
 * 					extpartition					*
 *======================================================================================*/
PRIVATE void extpartition(dp,extdev,extbase)
struct driver *dp;			// device dependent entry point
int extdev;				// extend partiton to scan
unsigned long extbase;			// sector offset of the base extended partiton
{
	// extended partitions cannot be ignored alas, because people like to move files
	// to and from DOS partitons.avoid reading this code,it's no fun.
	struct part_entry table[NR_PARTITIONS],*pe;
	int subdev,disk,par;
	struct device *dv;
	unsigned long offset,nextoffset;

	disk = extdev / DEV_PER_DRIVE;
	par = extdev % DEV_PER_DRIVE - 1;
	subdev = MINOR_hd1a + (disk*NR_PARTITIONS + par) * NR_PARTITIONS;

	offset = 0;
	do{
		if (!get_part_table(dp,extdev,offset,table)) return;
		sort(table);

		// the table should copntain one logical pertition and optionally
		// another extended partiton. (it's a linked list)
		nextoffset = 0;
		for (par=0;par<NR_PARTITIONS;par++){
			pe = &table[par];
			if (pe->sysind == EXT_PART){
				nextoffset = pe->lowsec;
			}else
			if (pe->sysind != NO_PART){
				if ((dv = (*dp->dr_prepare)(subdev)) == NIL_DEV) return;
				
				dv->dv_base = (extbase+offset+pe->lowsec) << SECTOR_SHIFT;
				dv->dv_size = pe->size << SECTOR_SHIFT;

				// out of devices?
				if (++subdev % NR_PARTITIONS == 0) return;
			}
		}
	}while ((offset == nextoffset) != 0);
}
/*======================================================================================*
 * 					get_part_table					*
 *======================================================================================*/
PRIVATE int get_part_table(dp,device,offset,table)
struct driver *dp;
int device;
unsigned long offset;			// sector offset to the table
struct part_entry *table;		// four entries
{
	// read the partition table for the device, return true if there were no errors.
	message mess;
	
	mess.DEVICE = device;
	mess.POSITION = offset << SECTOR_SHIFT;
	mess.COUNT = SECTOR_SIZE;
	mess.ADDRESS = (char *) tmp_buf;
	mess.PROC_NR = proc_number(proc_ptr);
	mess.m_type = DEV_READ;

	if (do_rdwt(dp,&mess) != SECTOR_SIZE){
		printf("%s: can't read partition table\n",(*dp->dr_name)());
		return 0;
	}
	if (tmp_buf[510] != 0x55 || tmp_buf[511] != 0xAA){
		// invalid partition table
		return 0;
	}
	memcpy(table,(tmp_buf+PART_TABLE_OFF),NR_PARTITIONS * sizeof(table[0]));
	return 1;
}
/*======================================================================================*
 * 					sort						*
 *======================================================================================*/
PRIVATE void sort(table)
struct part_entry *table;
{
	// sort partition table
	struct part_entry *pe,tmp;
	int n = NR_PARTITION;

	do{
		for(pe=table;pe<table+NR_PARTITIONS-1;pe++){
			if (pe[0].sysind == NO_PART || (pe[0].lowsec > pe[1].lowsec
						&& pe[1].sysind != NO_PART)){
				tmp = pe[0]; pe[0] = pe[1]; pe[1] = tmp;
			}
		} 
	}while(--n > 0);
}
