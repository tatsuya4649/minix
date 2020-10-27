// this files is master header of kernel.
// this files includes some other files and defines key constants.
//
//

#define _POSIX_SOURCE		1		// include POSIX stuff in header
#define _MINIX			1		// include MINIX stuff in header
#define _SYSTEM			1		// indicate in the header that this is the kernel

// the following is very basic,and the *.c file will
// automatically populate the following.
#include <minix/config.h>			// must be written at first
#include <ansi.h>				// must be written at second
#include <sys/types.h>
#include <minix/const.h>
#include <minix/type.h>
#include <minix/syslib.h>

#include <string.h>
#include <limits.h>
#include <errno.h>

#include "const.h"
#include "type.h"
#include "proto.h"
#include "glo.h"
