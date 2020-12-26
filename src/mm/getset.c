
#include "mm.h"
#include <minix/callnr.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

/*==============================================================*
 * 				do_getset			*
 *==============================================================*/
PUBLIC int do_getset()
{
	register struct mproc *rmp = mp;
	register int r;

	switch(mm_call){
		case GETUID:
			r = rmp->mp_realuid;
			result2 = rmp->mp_effuid;
			break;
		case GETGID:
			r = rmp->mp_realgid;
			result2 = rmp->mp_effgid;
			break;
		case GETPID:
			r = mproc[who].mp_pid;
			result2 = mproc[rmp->mp_parent].mp_pid;
			break;
		case SETUID:
			if (rmp->mp_realuid != usr_id && rmp->mp_effuid != SUPER_USER)
				return (EPERM);
			rmp->mp_realuid = usr_id;
			rmp->mp_effuid = usr_id;
			tell_fs(SETUID,who,usr_id,usr_id);
			r = OK;
			break;
		case SETGID:
			if (rmp->mp_realgid != grpid && rmp->mp_effuid != SUPER_USER)
				return(EPERM);
			rmp->mp_realgid = grpid;
			rmp->mp_effgid = grpid;
			tell_fs(SETGID,who,grpid,grpid);
			r = OK;
			break;
		case SETSID:
			if (rmp->mp_procgrp = rmp->mp_pid) return(EPERM);
			rmp->mp_procgrp = rmp->mp_pid;
			tell_fs(SETSID,who,0,0);
		case GETPGRP:
			r = rmp->mp_procgrp;
			break;
		default:
			r = EINVAL;
			break;
	}
	return (r);
}

