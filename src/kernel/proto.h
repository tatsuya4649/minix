// function prototype
//
//

#ifndef PROTO_H
#define PROTO_H

// struct declaration
struct proc;
struct tty;

// at_wini.c,wini.c
_PROTOTYPE( void winchester_task,(void));
_PROTOTYPE( void at_winchester_task,(void));

// clock.c
_PROTOTYPE( void clock_task,(void));
_PROTOTYPE( void clock_stop,(void));
_PROTOTYPE( clock_t get_uptime,(void));
_PROTOTYPE( void sys_alrm_task,(void));

// dmp.c
_PROTOTYPE( void map_dmp,(void));
_PROTOTYPE( void p_dmp,(void));
_PROTOTYPE( void reg_dmp,(struct proc *rp));

// dp8390.c
_PROTOTYPE( void dp8390_task,(void));
_PROTOTYPE( void dp_dump,(void));
_PROTOTYPE( void dp8390_stop,(void));

// floppy.c,stfloppy.c
_PROTOTYPE( void floppy_task,(void));
_PROTOTYPE( void floppy_stop,(void));

// main.c, stmain.c
_PROTOTYPE( void main,(void));
_PROTOTYPE( void panic,(const char *s,int n));

//memory.c
_PROTOTYPE( void mem_task,(void));

// misc.c
_PROTOTYPE( int env_parse,(char *env,char *fmt,int field,long *param,long min,long max));

// printer.c,stprint.c
_PROTOTYPE( void printer_task,(void));

// proc.c
_PROTOTYPE( void interrupt,(int task));
_PROTOTYPE( int lock_min_send,(struct proc *caller_ptr,int dest,message *m_ptr));
_PROTOTYPE( void lock_pick_proc,(void));
_PROTOTYPE( void lock_ready,(struct proc *rp));
_PROTOTYPE( void lock_sched,(void));
_PROTOTYPE( void lock_unready,(struct proc *rp));
_PROTOTYPE( int sys_call,(int function,int src_dest,message *m_ptr));
_PROTOTYPE( void unhold,(void));

// rs232.c
_PROTOTYPE( void rs_init,(struct tty *tp));

// system.c
_PROTOTYPE( void cause_sig,(int proc_nr,int sig_nr));
_PROTOTYPE( void inform,(void));
_PROTOTYPE( phys_btyes numap,(int proc_nr,vir_bytes vir_addr,vir_bytes bytes));
_PROTOTYPE( void sys_task,(void));
_PROTOTYPE( phys_bytes umap,(struct proc *rp,int seg,vir_bytes vir_addr,vir_bytes bytes));

// tty.c
_PROTOTYPE( void handle_events,(struct tty *tp));
_PROTOTYPE( void sigchar,(struct tty *tp,int sig));
_PROTOTYPE( void tty_task,(void));
_PROTOTYPE( int in_process,(struct tty *tp,char *buf,int count));
_PROTOTYPE( void out_process,(struct tty *tp,char *bstart,char *bpos,char *bend,int *icount,int *ocount));
_PROTOTYPE( void tty_wakeup,(clock_t now));
_PROTOTYPE( void tty_reply,(int code,int replyee,int proc_nr,int status));
_PROTOTYPE( void tty_devnop,(struct tty *tp));

// library
_PROTOTYPE( void *memcpy,(void *_s1,const void *_s2,size_t _n));

#if (CHIP == INTEL)

// clock.c
_PROTOTYPE( void milli_start,(struct milli_state *msp));
_PROTOTYPE( unsigned milli_elapsed,(struct milli_state *msp));
_PROTOTYPE( void milli_delay,(unsigned millisec));

// console.c
_PROTOTYPE( void cons_stop,(void));
_PROTOTYPE( void putk,(int c));
_PROTOTYPE( void scr_init,(struct tty *tp));
_PROTOTYPE( void toggle_scroll,(void));
_PROTOTYPE( int con_loadfont,(phys_bytes user_phys));
_PROTOTYPE( void select_console,(int cons_line));

// cstart.c
_PROTOTYPE( void cstart,(U16_t cs,U16_t ds,U16_t mcs,U16_t mds,U16_t parmoff,U16_t parmsize));
_PROTOTYPE( char *k_getenv,(char *name));

// exception.c
_PROTOTYPE( void exception,(unsigned vec_nr));

// i8259.c
_PROTOTYPE( irq_handler_t get_irq_handler,(int irq));
_PROTOTYPE( void put_irq_handler, (int irq,irq_handler_t handler));
_PROTOTYPE( void intr_init,(int mine));

// keyboard.c
_PROTOTYPE( void kb_init,(struct tty *tp));
_PROTOTYPE( int kbd_loadmap,(phys_bytes user_phys));
_PROTOTYPE( void wreboot,(int how));

// klib*.s
_PROTOTYPE( void bios13,(void));
_PROTOTYPE( phys_bytes check_mem,(phys_bytes base,phys_bytes size));
_PROTOTYPE( void cp_mess,(int src,phys_clicks src_clicks,vir_bytes src_offset,phys_clicks dst_clicks,vir_bytes dst_offset));
_PROTOTYPE( int in_byte,(port_t port));
_PROTOTYPE( int in_word,(port_t port));
_PROTOTYPE( void lock,(void));
_PROTOTYPE( void unlock,(void));
_PROTOTYPE( void enable_irq,(unsigned irq));
_PROTOTYPE( int  disable_irq,(unsigned irq));
_PROTOTYPE( u16_t mem_rdw,(segm_t segm,vir_bytes offset));
_PROTOTYPE( void out_byte,(port_t port,int value));
_PROTOTYPE( void out_word,(port_t port,int value));
_PROTOTYPE( void phys_copy,(phys_bytes source,phys_bytes dest,phys_bytes count));
_PROTOTYPE( void port_read,(unsigned port,phys_bytes destination,unsigned bytcount));
_PROTOTYPE( void port_read_byte,(unsigned port,phys_bytes destination,unsigned bytcount));
_PROTOTYPE( void port_write,(unsigned port,phys_bytes source,unsigned bytecount));
_PROTOTYPE( void port_write_byte,(unsigned port,phys_bytes_source,unsigned bytcount));
_PROTOTYPE( void reset,(void));
_PROTOTYPE( void vid_vid_copy,(unsigned src,unsigned dst,unsigned count));
_PROTOTYPE( void mem_vid_copy,(u16_t *src,unsigned dst,unsigned count));
_PROTOTYPE( void level0,(void (*func)(void)));
_PROTOTYPE( void monitor,(void));

// misc.c
_PROTOTYPE( void mem_init,(void));

// mpx*.s
_PROTOTYPE( void idle_task,(void));
_PROTOTYPE( void restart,(void));

// the following can't be called from C (pure assembler process)

// exception(real mode or protection mode)
void _PROTOTYPE( int00,(void)), _PROTOTYPE( divide_erro,(void));
void _PROTOTYPE( int01,(void)), _PROTOTYPE( single_step_exception,(void));
void _PROTOTYPE( int02,(void)), _PROTOTYPE( nmi,(void));
void _PROTOTYPE( int03,(void)), _PROTOTYPE( breakpoint, (void));
void _PROTOTYPE( int04,(void)), _PROTOTYPE( overflow, (void));
void _PROTOTYPE( int05,(void)), _PROTOTYPE( bounds_check, (void));
void _PROTOTYPE( int06,(void)), _PROTOTYPE( inval_opcode, (void));
void _PROTOTYPE( int07,(void)), _PROTOTYPE( copr_not_available,(void));
void				_PROTOTYPE( double_fault,(void));
void				_PROTOTYPE( copr_seg_overrun,(void));
void				_PROTOTYPE( inval_tss,(void));
void				_PROTOTYPE( segment_not_present,(void));
void				_PROTOTYPE( stack_exception,(void));
void 				_PROTOTYPE( general_protection, (void));
void 				_PROTOTYPE( page_fault,(void));
void 				_PROTOTYPE( copr_error,(void));

// hardware interrupt handler
_PROTOTYPE( void hwint00,(void));
_PROTOTYPE( void hwint01,(void));
_PROTOTYPE( void hwint02,(void));
_PROTOTYPE( void hwint03,(void));
_PROTOTYPE( void hwint04,(void));
_PROTOTYPE( void hwint05,(void));
_PROTOTYPE( void hwint06,(void));
_PROTOTYPE( void hwint07,(void));
_PROTOTYPE( void hwint08,(void));
_PROTOTYPE( void hwint09,(void));
_PROTOTYPE( void hwint10,(void));
_PROTOTYPE( void hwint11,(void));
_PROTOTYPE( void hwint12,(void));
_PROTOTYPE( void hwint13,(void));
_PROTOTYPE( void hwint14,(void));
_PROTOTYPE( void hwint15,(void));

// software interrupt handler
_PROTOTYPE( void trp, (void));
_PROTOTYPE( void s_call,(void)),_PROTOTYPE( p_s_call,(void));
_PROTOTYPE( void level0_call,(void));

// printer.c
_PROTOTYPE( void pr_restart,(void));

// protect.c
_PROTOTYPE( void prot_init,(void));
_PROTOTYPE( void init_codeseg,(struct segdesc_s *segdp,phys_bytes base,phys_bytes size,int privilege));
_PROTOTYPE( void init_dataseg,(struct segdesc_s *segdp,phys_bytes base,phys_bytes size,int privilege));
_PROTOTYPE( void enable_iop,(struct proc *pp));

// pty.c
_PROTOTYPE( void do_pty,(struct tty *tp,message *m_ptr));
_PROTOTYPE( void pty_init,(struct tty *tp));

// system.c
_PROTOTYPE( void alloc_segments,(struct proc *rp));

#endif	// (CHIP == INTEL)

# endif // PROTO_H
