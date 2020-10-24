


#ifndef _TYPE_H
#define _TYPE_H
#ifndef _MINIX_TYPE_H
#define _MINIX_TYPE_H

// define types
typedef unsigned int vir_clicks;	// virtual address and length clicks
typedef unsigned int phys_bytes;	// physical address and number of bytes in length
typedef unsigned int phys_clicks;	// physical address and number of clicks in length

#if (CHIP == INTEL)
typedef unsigned int vir_bytes;		// virtual address and number of bytes in length
#endif // CHIP == INTEL

#if (CHIP == M68000)
typedef unsigned int vir_bytes;		// virtual address and number of bytes in length
#endif // CHIP == M68000

#if (CHIP == SPARC)
typedef unsigned int vir_bytes;		// virtual address and number of bytes in length
#endif // CHIP == SPARC

// type associated with the message
#define M1		1
#define M3		3
#define M4		4
#define M3_STRING	14

typedef struct {int m1i1,m1i2,m1i3; char *m1p1,*m1p2,*m1p3;} mess_1;
typedef struct {int m2i1,m2i2,m2i3; long m2l1,m2l2; char *m2p1;} mess_2;
typedef struct {int m3i1,m3i2; char *m3p1; char m3ca1[M3_STRING];} mess_3;
typedef struct {long m4l1,m4l2,m4l3,m4l4,m4l5;} mess_4;
typedef struct {char m5c1,m5c2; int m5i1,m5i2; long m5l1,m5l2,m5l3;} mess_5;
typedef struct {int m6i1,m6i2,m6i3; long m6l1; sighandler_t m6f1;} mess_6;

typedef struct {
	int m_source;		// message sender
	int m_type;		// type of message
	union{
		mess_1 m_m1;
		mess_2 m_m2;
		mess_3 m_m3;
		mess_4 m_m4;
		mess_5 m_m5;
		mess_6 m_m6;
	} m_u;
} message;

// define usefule member names below
#define m1_i1 m_u.m_m1.m1i1
#define m1_i2 m_u.m_m1.m1i2
#define m1_i3 m_u.m_m1.m1i3
#define m1_p1 m_u.m_m1.m1p1
#define m1_p2 m_u.m_m1.m1p2
#define m1_p3 m_u.m_m1.m1p3

#define m2_i1 m_u.m_m2.m2i1
#define m2_i2 m_u.m_m2.m2i2
#define m2_i3 m_u.m_m2.m2i3
#define m2_l1 m_u.m_m2.m2l1
#define m2_l2 m_u.m_m2.m2l2
#define m2_p1 m_u.m_m2.m2p1

#define m3_i1 m_u.m_m3.m3i1
#define m3_i2 m_u.m_m3.m3i2
#define m3_p1 m_u.m_m3.m3p1
#define m3_ca1 m_u.m_m3.m3ca1

#define m4_l1 m_u.m_m4.m4l1
#define m4_l2 m_u.m_m4.m4l2
#define m4_l3 m_u.m_m4.m4l3
#define m4_l4 m_u.m_m4.m4l4
#define m4_l5 m_u.m_m4.m4l5

#define m5_c1 m_u.m_m5.m5c1
#define m5_c2 m_u.m_m5.m5c2
#define m5_i1 m_u.m_m5.m5i1
#define m5_i2 m_u.m_m5.m5i2
#define m5_l1 m_u.m_m5.m5l1
#define m5_l2 m_u.m_m5.m5l2
#define m5_l3 m_u.m_m5.m5l3

#define m6_i1 m_u.m_m6.m6i1
#define m6_i2 m_u.m_m6.m6i2
#define m6_i3 m_u.m_m6.m6i3
#define m6_l1 m_u.m_m6.m6l1
#define m6_f1 m_u.m_m6.m6f1

struct mem_map{
	vir_clicks mem_vir;	// virtual address
	phys_clicks mem_phys;	// physical address
	vir_clicks mem_len;	// length
};

struct iorequest_s{
	long io_position;		// location in device file (actualyy off_t)
	char *io_buf;			// buffer of user space
	int io_nbytes;			// required size
	unsigned short io_request;	// read,write(option)
};
#endif // _TYPE_H

typedef struct{
	vir_bytes iov_addr;		// address of I/O buffer
	vir_bytes iov_size;		// size of I/O buffer
} iovec_t;

typedef struct{
	vir_bytes cpv_src;		// src address of data
	vir_bytes cpv_dst;		// dst address of data
	vir_bytes cpv_size;		// size of data
} cpvec_t;

// MM passes the address of this type of structure to KERNEL when do_sendsig() is
// called as part of the signal acquisition mechanism.
// this structure contains all the information that KERNEL needs to build a signal stack.
struct sigmsg{
	int sm_signo;			// signal number to be acquired
	unsigned long sm_mask;		// mask to restore when returning from handler
	vir_bytes sm_sighandler;	// address of handler
	vir_bytes sm_sigreturn;		// address of _sigreturn in C library
	vir_bytes sm_stkptr;		// user stack pointer
};

#define MESS_SIZE (sizeof(message))	// it may be necessary to usizeof from fs
#define NIL_MESS ((message*) 0)

struct psinfo{ // info used in ps(1) program
	u16_t nr_tasks,nr_procs;	// NR_TASKS and NR_PROCS constant
	vir_bytes proc,mproc,fproc;	// address of main process table
};

#endif // _MINIX_TYPE_H
