
#ifndef _TABLE
#undef EXTERN
#define EXTERN
#endif

EXTERN struct mproc *mp;
EXTERN int dont_reply;
EXTERN int procs_in_use;

EXTERN message mm_in;
EXTERN message mm_out;
EXTERN int who;
EXTERN int mm_call;

EXTERN int err_code;
EXTERN int result2;
EXTERN char *res_ptr;

extern _PROTOTYPE(int (*call_vec[]),(void));
extern char core_name[];
EXTERN sigset_t core_sset;
