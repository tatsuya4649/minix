// <ibm/partition.h> header that includes partition entory list.
//
//

#ifndef _PARTITION_H
#define _PARTITION_H

struct part_entry{
	unsigned char bootind;		// boot indicator0/ACTIVE_FLAG
	unsigned char start_head;	// start value of first sector
	unsigned char start_sec;	// sector value of first sector + cyl bit
	unsigned char start_cyl;	// track value of first sector
	unsigned char sysind;		// system indicator
	unsigned char last_head;	// start value of last sector
	unsigned char last_sec;		// sector value of last sector + cyl bit
	unsigned char last_cyl;		// track value of last sector
	unsigned long lowsec;		// logical first sector
	unsigned long size;		// partition size of sector unit
};	

#define ACTIVE_FLAG	0x80		// active value of boot indicator(hd0) field
#define NR_PARTITIONS	4		// number of entory on partition table
#define PART_TABLE_OFF	0x1BE		// boot sector partition table offset

// type of partition
#define MINIX_PART	0x81		// partition type of minix
#define NO_PART		0x00		// unused entries
#define OLD_MINIX_PART	0x80		// now unused entries
#define EXT_PART	0x05		// extension partition

#endif // _PARTITION_h
