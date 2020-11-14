// IBM device driver define
//

#include <ibm/partition.h>

_PROTOTYPE(void partition, (struct driver *dr,int device,int style));

// BIOS parameter table layout
#define bp_cylinders(t)			(* (u16_t *) (&(t)[0]))
#define bp_heads(t)			(* (u8_t  *) (&(t)[2]))
#define bp_reduced_wr(t)		(* (u16_t *) (&(t)[3]))
#define bp_precomp(t)			(* (u16_t *) (&(t)[5]))
#define bp_max_ecc(t)			(* (u8_t  *) (&(t)[7]))
#define bp_ctlbyte(t)			(* (u8_t  *) (&(t)[8]))
#define bp_landingzone(t)		(* (u16_t *) (&(t)[12]))
#define bp_sectors(t)			(* (u8_t  *) (&(t)[14]))

// other
#define DEV_PER_DRIVE			(1 + NR_PARTITIONS)
#define MINOR_hd1a			128
#define MINOR_fd0a			(28<<2)
#define P_FLOPPY			0
#define P_PRIMARY			1
#define P_SUB				2
