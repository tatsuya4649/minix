// <minix/partition.h> header used in DIOCGETP and DIOCSETP ioctl.
// disk partition location and disk placement.
//
//

#ifndef _MINIX_PARTITION_H
#define _MINIX_PARTITION_H

struct partition{
	u32_t base;		// byte offset at partision start position
	u32_t size;		// size of partition(byte)
	unsigned cylinders;	// disc placement
	unsigned heads;
	unsigned sectors;
};

#endif // _MINIX_PARTITION_H
