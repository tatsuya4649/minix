// <minix/boot.h> header is used by both kernel and filesystem to define the device and access
// the parameters that the boot program process to the system.
//
//

#ifndef _BOOT_H
#define _BOOT_H

// redefine root and root directory as variables.
// this keeps the diffs small,but causes confusion.
#define ROOT_DEV (boot_parameters.bp_rootdev)
#define IMAGE_DEV (boot_parameters.bp_ramimagedev)

// the device number h/com.h of RAM,floppy,and hard disk device defines RAM_DEV,
// but only the minor number.
#define DEV_FD0		0x200
#define DEV_HD0		0x300
#define DEV_RAM		0x100
#define DEV_SCSI	0x700		// only Atari TT

// struct to hold boot parameter
struct bparam_s
{
	dev_t bp_rootdev;
	dev_t bp_ramimagedev;
	unsigned short bp_ramsize;
	unsigned hsort bp_processor;
};

extern struct bparam_s boot_parameters;

#endif // _BOOT_H
