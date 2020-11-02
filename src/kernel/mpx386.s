#
! this file contains the assembler startup code for the minix and 32-bit
! interrupt handlers. this file,along with start.c,sets up a good environment for main()

! this file is part of the lower layer of the minix kernel.the other part is "proc.c".
! the lowest layer performs process switching and message processing.

! all transport to the kernel is done through this file.
! transport is triggerd by sending and receiving message and interrupts
! (RS232 interrupts can be handled by the file "rs2.s" and rarely enter the kernel).

! transfers to the kernel can be nested.initial entries are system calls,
! exceptions,or hardware interrupts,and reentry can only be done by hardware.
! the reentry count is stored in "k_reenry".this is important for deciding whether to switch
! to kernel studs and for protecting the message passcode in "proc.c".

! for message path traps,most of the machine state is stored in the proc tabl
! (some of the registers do not need to be stored).
! the stack is then switched to "k_stack" and interrupts are re-enbled.
! finally,the system code handler is called.upon returning from the handler,the interrupt is
! disabled again, the pending interrupt is terminated,and the process or task with the pointer
! "proc_ptr" is executed.

! hardware interrupt handlers do the same.however,
! (1) the entrie state must be saved.
! (2) there are too many handlers to do this inline,so the save routin is called.several cycles
!     are saved by pushing the address of the appropriate restart routine for the return to be followed.
! (3) if the stack has already been switched, the stack switch is avoided.
! (4) (Master) 8259 the interrupt controller is made available again by save().
! (5) use 8259 to mask the interrupt line and unmask it after servicing the interrupt before each 
!     interrupt handler enables interrupt m(without other mask). this limits the nesting level to
!     the number of lines and protects the handler from itself.

! certain constant deata is compiled at the begining of the text segment for communication
! with the boot monitor at startup.
! this is a data read function at the start of the boot process,as it only needs to read
! the first sector of the file.

! a certain amount of data storage is also allocated at the end of this file.
! this data is at the begining of the kernel's data segment and is read and
! modified by the boot monitor befor the kernel starts.

! section

.sect .text
begtext:
.sect .rom
begrom:
.sect .data
begdata:
.sect .bss
begbss:

#include <minix/config.h>
#include <minix/const.h>
#include <minix/com.h>
#include "const.h"
#include "protect.h"
#include "sconst.h"


/* selected 386 tss offset */
#define TSS3_S_SP0		4

! export function
! note: in assembly language, the .define statement applied to each function is
! roughly equivalent to a C code prototype.
! Allows you to link to entities declared in assembly code,but no entities are created.

.define _idle_task
.define _restart
.define save

.define _divide_error
.define _single_step_exception
.define _nmi
.define _breakpoint_exception
.define _overflow
.define _bounds_check
.define _inval_opcode
.define _copr_not_available
.define _double_fault
.define _copt_seg_overrun
.define _inval_tss
.define _segment_not_present
.define _stack_exception
.define _general_protection
.define _page_fault
.define _copr_error

.define _hwint00		! hardware interrupt handler
.define _hwint01
.define _hwint02
.define _hwint03
.define _hwint04
.define _hwint05
.define _hwint06
.define _hwint07
.define _hwint08
.define _hwint09
.define _hwint10
.define _hwint11
.define _hwint12
.define _hwint13
.define _hwint14
.define _hwint15

.define _s_call
.define _p_s_call
.define _level0_call

! inport function

.extern _cstart
.extern _main
.extern _exception
.extern _iterrupt
.extern _sys_call
.extern _unhold

! export variables
! note : if you use .define for a variable, it will not reserve strage and will
! 	show the name to the outside,so it can be linked.

.define begbss
.define begdata
.define _sizes

! inport variables

.extern _gdt
.extern _code_base
.extern _data_base
.extern _held_head
.extern _k_reenter
.extern _pc_at
.extern _proc_ptr
.extern _ps_mca
.extern _tss
.extern _level0_func
.extern _mon_sp
.extern _mon_return
.extern _reboot_code

.sect .text
! *=====================================================================*
! * 				MINIX					*
! *=====================================================================*
MINIX:					! entry point of MINIX kernel
	jmp	over_flags		! skip over the next few bytes
	.data2	CLICK_SHIFT		! for monitor : memory fine particle size
flags:
	.data2	0x002D			! boot monitor flags:
					! 386 mode load,stack generation,high load
	nop				! spare bite to normalize disassembler
over_flags:

! set the C stack frame in the monitor stack ( the monitor sets cs and ss correctly.
! the ss descriptor still refers to the monitor data segment.)
	movzx	esp,sp			! the monitor stack is a 16-bit stack
	push	ebp
	mov	ebp,esp
	push 	esi
	push	edi
	cmp	4(ebp), 0		! non-zero if recoverable
	jz	noret
	inc 	(_mon_return)
noret:	mov	(_mon_sp),esp		! save stack point for recovering

! copy the monitor global descriptor table to the kernel address space
! and switch to it. this allows Prot_init() to update it immediately.

	sgdt	(_gdt+GDT_SELECTOR)		! get monitor gdtr
	mov	esi, (_gdt+GDT_SELECTOR+2)	! absolute address of GDT
	mov	ebx,_gdt			! address of kernel GDT
	mov	ecx, 8*8			! copy 8 descriptor
copygdt:
 eseg	movb	al,(esi)
	movb	(ebx),al
	inc	esi
	inc	ebx
	loop	copygdt
	mov	eax,(_gdt+DS_SELECTOR+2)	! base of kernel data
	and	eax,0x00FFFFFF			! only 24-bit
	add	eax,_gdt
	mov	(_gdt+GDT_SELECTOR+2),eax	! set base of GDT
	lgdt	(_gdt+GDT_SELECTOR)		! switch kernel GDT

! position boot parameters and set kernel segment registers and stack
	mov	ebx,8(ebp)			! boot parameter offset
	mov	edx,12(ebp)			! boot parameter length
	mov	ax,ds				! kernel data
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax
	mov	esp,k_stktop			! set sp for pointing top of kernel stack

! call the C startup code and set up the appropriate environment to run main()
	push	edx
	push	ebx
	push	SS_SELECTOR
	push	MON_CS_SELECTOR
	push	DS_SELECTOR
	push	CS_SELECTOR
	call	_cstart				! cstart(cs,ds,mcs,mds,parmoff,parmlen)
	add	esp, 6*4

! reload gdtr,idtr and segment registers into the global descriptor table set by prot_init()

	lgdt	(_gdt+GDT_SELECTOR)
	lids	(_gdt+IDT_SELECTOR)

	jmpf	CS_SELECTOR:csinit
csinit:
    o16 mov	ax,DS_SELECTOR
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax
    o16 mov	ax,TSS_SELECTOR			! use only TSS
	ltr	ax
	push	0				! flag to known appropriate state
	popf					! esp,clear nested tasks,int enabled
	
	jmp	_main				! main()

! *=====================================================================================*
! *				interrupt handlers					*
! *			interrupt handlers for 386 32-bit protected mode		*	
! *=====================================================================================*

! *=====================================================================================*
! *				hwint00 - 07						*
! *=====================================================================================*
! lools like a subroutine,but is a macro.
#define hwint_master(irq)	\
		call	save;			/* save interrupted process stat */
		inb	INT_CTLMASK;
		orb	al,[1<<irq];
		outb	INT_CTLMASK;		/* invalid IRQ */
		movb	al,ENABLE;
		outb	INT_CTL;		/* re-enable master8259 */
		sti	;			/* enable interrupt */
		push	irq;			/* irq */
		call	(_irq_table+4*irq);	/* eax = (*irq_table[irq])(irq) */
		pop	ecx;
		cli	;			/* invalid interrupt */
		test	eax,eax;		/* do I need to re-enable IRQ */
		jz	0f;
		inb	INT_CTLMASK;
		andb	al,[1<<irq];
		outb	INT_CTLMASK;		/* enable IRQ */
0:		ret				/* restart (another) process */

! each entry point is an extension of the hwint_master macro
		.align	16
_hwint00:			! a interrupt routine of IRQ 0(clock)
		hwint_master(0)

		.align	16
_hwint01:			! a interrupt routine of IRQ 1(keyboard)
		hwint_master(1)
		
		.align	16
_hwint02:			! a interrupt routine of IRQ 2(cascade!)
		hwint_master(2)

		.align	16
_hwint03:			! a interrupt routine of IRQ 3(seconde cereal)
		hwint_master(3)

		.align	16
_hwint04:			! a interrupt routine of IRQ 4(first careal)	
		hwint_master(4)

		.align	16
_hwint05:			! a interrupt routine of IRQ 5(XT winchester)
		hwint_master(5)

		.align	16
_hwint06:			! a interrupt routine of IRQ 6(floppy)
		hwint_master(6)

		.align  16
_hwint07:			! a interrupt routine of IRQ 7(printer)
		hwint_master(7)

		.align	16

! *=====================================================================================*
! *					hwint08 - 15					*
! *=====================================================================================*
! looks like a subroutine but is a macro
#define hwint_slave(irq)	\
	call	save;			/* save interrupt process state */
	inb	INT2_CTLMASK;
	orb	al,[1<<[irq-8]];
	outb	INT2_CTLMASK;		/* invalid IRQ */
	movb	al,ENABLE;
	outb	INT_CTL;		/* re-enable master8259	*/
	jmp	.+2;			/* delay */
	outb	INT2_CTL;		/* re-enable slave8259 */
	sti	;			/* enable interrupt */
	push	irq;			/* irq	*/
	call	(_irq_table+4*irq);	/* eax = (*irq_table[irq])(irq)
	pop	ecx;
	cli	;			/* disable interrupt
	test	eax,eax			/* do I need to re-enable IRQ
	jz	0f;
	inb	INT2_CTLMASK;
	andb	al,~[1<<[irq-8]];
	outb	INT2_CTLMASK;		/* enable IRQ */
0:	ret				/* re-start (another) process */

! each entry point is an extension of the hwint_slave macro
	.align	16
_hwint08:		! a interrupt routine of IRQ 8(real time clock)
	hwint_slave(8)

	.align	16
_hwint09:		! a interrupt routine of IRQ 9(redirect irq2)
	hwint_slave(9)

	.align	16
_hwint10:		! a interrupt routine of IRQ 10
	hwint_slave(10)

	.align	16
_hwint11:		! a interrupt routine of IRQ 11
	hwint_slave(11)

	.align	16
_hwint12:		! a interrupt routine of IRQ 12
	hwint_slave(12)

	.align	16
_hwint13:		! a interrupt routine of IRQ 13 (FPU exception)
	hwint_slave(13)

	.align	16
_hwint14:		! a interrupt routine of IRQ 14 (AT winchester)
	hwint_slave(14)

	.align	16
_hwint15:		! a interrupt routine of IRQ 15
	hwint_slave(15)

	.align	16

! *=====================================================================================*
! *					save						*
! *=====================================================================================*
! save for protection mode
! this is much simpler than 8086 mode, as the stack is already pointing to
! the process table or has already been switched to the kernel stack.

	.align	16
save:
	cld			! set direction flag to a known value
	pushad			! save "general" register
    o16	push	ds		! save ds
    o16	push	es		! save es	
    o16	push	fs		! save fs	
    o16	push	gs		! save gs	
	mov	dx,ss		! ss is a kernel data segment.
	mov	ds,dx		! load the rest of the kernel segment.
	mov	es,dx		! kernel does not use fs,gs
	mov	eax,esp		! get ready to return
	incb	(_k_reenter)	! from -1 if don't re-execute
	jnz	set_restart1	! stack is already kernel stack
	mov	esp,k_stktop
	push	_restart	! construct erturn address for int handler
	xor	ebp,ebp		! for stack trace
	jmp	RETADR-P_STACKBASE(eax)
	
	.align	4
set_restart1:
	push	restart1
	jmp	RETADR-P_STACKBASE(eax)


! *=====================================================================================*
! *					_s_call						*
! *=====================================================================================*
	.align	16
_s_call:
_p_s_call:
	cld			! set the direction flag to a known value
	sub	esp, 6*4	! skip RETADR,eax,ecx,edx,ebx,est
	push	ebp		! the stack already points to the proc table
	push	esi
	push	edi
    o16 push	ds
    o16 push	es
    o16 push	fs
    o16 push	gs
	mov	dx,ss
	mov	ds,dx
	mov	es,dx
	incb	(_k_reenter)
	mov	esi,esp		! assume P_STACKBASE == 0
	mov	esp,k_stktop
	xor	ebp,ebp		! for stack trace
				! last of inline save
	sti			! allot to interrupt SWITCHER
				! set parameter for sys_call() here
	push	ebx		! pointer to user message
	push	eax		! src/dest
	push	ecx		! SEND/RECEIVE/BOTH
	call	_sys_call	! sys_call(function,src_dest,m_ptr)
				! the caller is now explicitly in proc_ptr
	mov	AXREG(esi),eax	! sys_call must hold si
	cli			! disable interrupt

! continue to the code for restarting excution of proc/task

! *=====================================================================================*
! *					restart						*
! *=====================================================================================*
_restart:

! flush pending interrupts.
! this re-enables the interrupt so that the current interrupt handler can be re-executed.
! this is not a problem as the current handler is about to exit and flushing only
! happends when k_reenter == 0,so no other handler can return.

	cmp	(_held_head),0		! do fast tests to avoid normal function calls
	jz	over_call_unhold
	call	_unhold			! overhead is acceptable as this is rare
over_call_unhold:
	mov	esp,(_proc_ptr)		! assume P_STACKBASE == 0
	lldt	P_LDT_SEL(esp)		! enable segment descriptor for task
	lea	eax,P_STACKTOP(esp)	! ready to interrupt next
	mov	(_tss+TSS3_S_SP0),eax	! save state on process table
restart1:
	decb	(_k_reenter)
    o16	pop	gs
    o16	pop	fs
    o16	pop	es
    o16	pop	ds
	popad
	add	esp,4			! skip return address
	iretd				! continue process

	
! *=====================================================================================*
! *				exception handlers					*
! *=====================================================================================*
_divide_error:
	push	DIVICE_VECTOR
	jmp	exception

_single_step_exception:
	push	DEBUG_VECTOR
	jmp	exception

_nmi:
	push	NMI_VECTOR
	jmp	exception

_breakpoint_exception:
	push	BREAKPOINT_VECTOR
	jmp	exception

_overflow:
	push	OVERFLOW_VECTOR
	jmp	exception

_bounds_check:
	push	BOUNDS_VECTOR
	jmp	exception

_inval_opcode:
	push	INVAL_OP_VECTOR
	jmp	exception

_copr_not_available:
	push	COPROC_NOT_VECTOR
	jmp	exception

_double_fault:
	push	DOUBLE_FAULT_VECTOR
	jmp	exception

_copr_seg_overrun:
	push	COPROC_SEG_VECTOR
	jmp	exception

_inval_tss:
	push	INVAL_TSS_VECTOR
	jmp	exception

_segment_not_present:
	push	SEG_NOT_VECTOR
	jmp	exception

_stack_exception:
	push	STACK_FAULT_VECTOR
	jmp	exception

_general_protection:
	push	PROTECTION_VECTOR
	jmp	errexception

_page_fault:
	push	PAGE_FAULT_VECTOR
	jmp	errexception

_copr_error:
	push	COPROC_ERR_VECTOR
	jmp	exception

! *=====================================================================================*
! *				exception						*
! *=====================================================================================*
! this is called for all exception that do not push the error code.
	.align	16
exception:
   sseg	mov	(trap_errno),0		! clear trap_errno
   sseg	pop	(ex_number)
	jmp	exception1

! *=====================================================================================*
! *				errexception						*
! *=====================================================================================*
! this is called for all exceptions that push the error code

	.align	16
errexception:
   sseg	pop	(ex_number)
   sseg	pop	(trap_errno)
exception1:				! common to all exceptions
	push	eax			! eax is scratch register
	mov	eax,0+4(esp)		! old eip
   sseg mov	(old_eip),eax
	movzx	eax,4+4(esp)		! old cs
   sseg	mov	(old_cs),eax
	mov	eax,8+4(esp)		! old eflags
   sseg	mov	(old_eflags),eax
	pop	eax
	call	save
	push	(old_eflags)
	push 	(old_cs)
	push	(old_eip)
	push	(trap_errno)
	push	(ex_number)
	push	_exception		! (ex_number,trap_errno,old_eip,old_cs,old_eflags)
	add	esp,5*4
	cli
	ret

! *=====================================================================================*
! *				level0_call						*
! *=====================================================================================*
_level0_call:
	call	save
	jmp	(_level0_func)

! *=====================================================================================*
! *				idle_task						*
! *=====================================================================================*
_idle_task:				! excuted when there is no work to be processed
	jmp	_idle_task		! "Hlt" before failing in protected mode


! *=====================================================================================*
! *				data							*
! *=====================================================================================*
! these decleration allocate storage at the beginning of the kernel data section,
! making it easy to show the boot monitor how to patch these locations. note that the
! compiler assigns a magic number here,but it is read by the boot monitor and then
! overwritten. you can see here that the kernel starts the size array as if it had been 
! initialized by the compiler.

.sect .rom		! before the string table
_sizes:				! kernel,mm,fs size entered by boot
	.data2	0x526F		! this should be the first data entry(magic #)
	.space	16*2*2-2	! the monitor uses the previous word and this space
				! spare space allow agging server
.sect .bss
k_stack:
	.space	K_STACK_BYTES	! kernel stack
k_stktop:			! top of kernel stack
	.comm	ex_number,4
	.comm	trap_errno,4
	.comm	old_eip,4
	.comm	old_cs,4
	.comm	old_eflags,4
