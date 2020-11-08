#
! section

.sect .text; .sect .rom; .sect .data; .sect .bss

#include <minix/config.h>
#include <minix/const.h>
#include "const.h"
#include "sconst.h"
#include "protect.h"

! this file contains a number of assembly code utility routines needed by the kernel.
! routines are as follows:

.define _monitor			! exit minix and return to the monitor
.define _check_mem			! check memory block, return effective size
.define _cp_mess			! copy message from source to destination
.define _exit				! dummy for library routines
.define __exit				! dummy for library routines
.define ___exit				! dummy for library routines
.define ___main				! dummy for GCC
.define _in_byte			! read 1 byte from the port and return it
.define _in_word			! read 1 word from the port and return it
.define _out_byte			! write 1 byte to the port
.define _out_word			! write 1 word to the port
.define _port_read			! (disk controller) transfer data from port to memory
.define _port_read_byte			! do the same for each byte
.define _port_write			! transfor data from memory to (disk controller) port
.define _port_write_byte		! do the same for each byte
.define _lock				! disable interrupts
.define _unlock				! enable interrupts
.define _enable_irq			! enable IRQ on the 8259 controller
.define _disable_irq			! disable irq
.define _phys_copy			! copy data from memory location to location
.define _mem_rdw			! copy 1 word from [Segment : Offset]
.define _reset				! reset system
.define _mem_vid_copy			! copy data to video RAM
.define _vid_vid_copy			! move data of video RAM
.define _level0				! call function with level 0

! the routines only guarantee to preserve the registers the C compiler
! expects to be preserved ( ebx,esi,edi,ebp,esp,segment registers and
! direction bit in the flags).

.sect .bss
.extern _mon_return,_mon_sp
.extern _irq_use
.extern _blank_colro
.extern _ext_memsize
.extern _gdt
.extern _low_memsize
.extern _sizes
.extern _vid_seg
.extern _vid_size
.extern _vid_mask
.extern _level0_func

.sect .text
!*==============================================================================*
!*					monitor					*
!*==============================================================================*
! PUBLIC void monitor();
! Return to the monitor.

_monitor:
	mov	eax,(_reboot_code)		! address of new parameter
	mov	esp,(_mon_sp)			! restore monitor stack pointer
    o16 mov	dx,SS_SELECTOR			! monitor data segment
	mov	ds,dx
	mov	es,dx
	mov	fs,dx
	mov	gs,dx
	mov	ss,dx
	pop	edi
	pop	esi
	pop	ebp
    o16	retf					! return monitor
 
!*==============================================================================*
!*					check_mem				*
!*==============================================================================*
! PUBLIC phys_bytes check_mem(phys_bytes base,phys_bytes size);
! checks for blocks of memory and returns a balid value
! check only the 16th byte each.
! initial size 0 means everything.
! this actually has to do an ailias check.

CM_DENSITY 		=	16
CM_LOG_DENSITY		=	4
TEST1PATTERN 		=	0x55		! memory test pettern 1
TEST2PATTERN		=	0xAA		! memory test pattern 2

CHKM_AGGS		=	4 + 4 + 4	! 4+4
!				ds ebx eip	base size

_check_mem:
	push	ebx
	push	ds
    o16	mov	ax,FLAT_DS_SELECTOR
	mov	ds,ax
	mov	eax,CHKM_ARGS(esp)
	mov	ebx,eax
	mov	ecx,CHKM_ARGS+4(esp)
	shr	ecx,CM_LOG_DENSITY
cm_loop:
	movb	dl,TEST1PATTERN
	xchgb	dl,(eax)			! write a test pattern and remember the original
	xchgb	dl,(eax)			! restore the original and load the test pattern
	cmpb	dk,TEST1PATTERN			! must match if correct real memory
	jnz	cm_exit				! if not,memory is unavailable
	movb	dl,TEST2PATTERN
	xchgb	dl,(eax)
	xchgb	dl,(eax)
	add	eax,CM_DENSITY
	cmpb	dl,TEST2PATTERN
	loopz	cm_loop
cm_exit:
	sub	eax,ebx
	pop	ds
	pop	ebx
	ret

!*==============================================================================*
!*					cp_mess					*
!*==============================================================================*
! PUBLIC void cp_mess(int src,phys_clicks src_clicks,vir_bytes src_offset,phys_clicks dst_clicks,
			vir_bytes dst_offset);
! this routine maskes a fast copy of a message from anywhere in the address
! space to anywhere else.it also copies the source address provided as a parameter to the
! call into the first word of the destination message.
!
! note that the message size,"Msize" is in DWORDS (not bytes) and must be set correctly.
! Changing the definition of message in the type file and not
! changing it here will lead to total disaster.

CM_ARGS =		4 + 4 + 4 + 4 + 4	! 4 + 4 + 4 + 4 + 4
			es  ds  edi esi eip	proc scl sof dcl dof
	.align	16
_cp_mess:
	cld
	push	esi
	push	edi
	push	ds
	push	es

	mov	eax,FLAT_DS_SELECTOR
	mov	ds,ax
	mov	es,ax

	mov	esi,CM_ARGS+4(esp)		! source click
	shl	esi,CLICK_SHIFT
	add	esi,CM_ARGS+4+4(esp)		! source offset
	mov	edi,CM_ARGS+4+4+4(esp)		! click of destination
	shl	edi,CLICK_SHIFT
	add	edi,CM_ARGS+4+4+4+4(esp)	! offset of destination

	mov	eax,CM_ARGS(esp)		! process number of destination
	stos					! copy number source to dest message
	add	esi,4				! not copy first word
	mov	ecx,Msize - 1			! remember that the first word does not count
	rep
	movs					! copy message

	pop	es
	pop	ds
	pop	edi
	pop	esi
	ret					! that's it


!*==============================================================================*
!*					exit					*
!*==============================================================================*
! PUBLIC void exit();
! some library routines use exit, so provide a dummy version.
! actual calls to exit cannot occur in the kernel.
! GNU CC likes to call __main from main() for nonobvious reasons.

_exit:
__exit:
___exit:
	sti
	jmp	___exit

__main:
	ret

!*==============================================================================*
!*					in_byte					*
!*==============================================================================*
! PUBLIC unsigned in_byte(port_t port);
! read (unsigned) bytes from the I/O port and returns them.
	
	.align 16
_in_word:
	mov	edx,4(esp)		! port
	sub	eax,eax
    o16	in	dx			! read 1 word
	ret

!*==============================================================================*
!*					out_byte				*
!*==============================================================================*
! PUBLIC void out_byte(port_t port,u8_t value);
! write a value (cast to bytes) to the I/O port

	.align	16
_out_byte:
	mov	edx,4(esp)		! port
	movb	al,4+4(esp)		! value
	outb	dx			! output 1 bytes
	ret

!*==============================================================================*
!*					out_word				*
!*==============================================================================*
! PUBLIC void out_word(Port_t port,U16_t value);
! write a value (cast to word) to the I/O port

	.align	16
_out_word:
	mov	edx,4(esp)		! port
	mov	edx,4+4(esp)		! value
    o16	out	dx			! 1 word output
	ret

!*==============================================================================*
!*					port_read				*
!*==============================================================================*
! PUBLIC void port_read(port_t port,phys_bytes destination,unsigned bytcount);
! (Hard disk controller) transfer data from port to memory

PR_ARGS =		4 + 4 + 4	! 4 + 4 + 4
!			es edi  eip     port dst  len

	.align	16
_port_read:
	cld
	push	edi
	push	es
	mov	ecx,FLAT_DS_SELECTOR
	mov	es,cx
	mov	edx,PR_ARGS(esp)	! read port
	mov	edi,PR_ARGS+4(esp)	! address of destination
	mov	ecx,PR_ARGS+4+4(esp)	! byte count
	shr	ecx,1			! word count
	rep				! ( hardware cannot handle dword )
   o16  ins				! read everything
	pop	es
	pop	edi
	ret


!*==============================================================================*
!*					port_read_byte				*
!*==============================================================================*
! PUBLIC void port_read_byte(port_t port,phys_bytes destination,unsigned bytcount);
! transfer data from port to memory

PR_ARGS_B =		4 + 4 + 4	! 4 + 4 + 4
!			es edi eip	 port dst len
_port_read_byte:
	cld
	push	edi
	push	es
	mov	ecx,FLAT_DS_SELECTOR
	mov	es,cx
	mov	edx,PR_ARGS_B(esp)
	mov	edi,PR_ARGS_B+4(esp)
	mov	ecx,PR_ARGS_B+4+4(esp)
	rep
	insb
	pop	es
	pop	edi
	ret


!*==============================================================================*
!*					port_write				*
!*==============================================================================*
! PUBLIC void port_write(port_t port,phys_bytes source,unsigned bytcount);
! transfer data from memory to (hard disk controller) port

PW_ARGS =		4 + 4 + 4	! 4 + 4 + 4
!			es edi  eip	port  src len

	.align	16
_port_write:
	cld
	push	esi
	push	ds
	mov	ecx,FLAT_DS_SELECTOR
	mov	ds,cx
	mov	edx,PW_ARGS(esp)	! write port
	mov	esi,PW_ARGS+4(esp)	! source address
	mov	ecx,PW_ARGS+4+4(esp)	! byte count
	shr	ecx,1			! word count
	rep				! (hardware cannot handle dword)
    o16 outs
	pop	ds
	pop	esi
	ret

!*==============================================================================*
!*					port_write_byte				*
!*==============================================================================*
! PUBLIC void port_write_byte(port_t port,phys_bytes souce,unsigned bytcount);
! 
! transfer data from memory to port

PW_ARGS_B =		4 + 4 + 4	! 4 + 4 + 4
!			es edi eip	port src len

_port_write_byte:
	cld
	push	esi
	push	ds
	mov	ecx,FLAT_DS_SELECTOR
	mov	ds,cx
	mov	edx,PW_ARGS_B(esp)
	mov	esi,PW_ARGS_B+4(esp)
	mov	ecx,PW_ARGS_B+4+4(esp)
	rep
	outsb
	pop	ds
	pop	esi
	ret


!*==============================================================================*
!*					lock					*
!*==============================================================================*
! PUBLIC void lock();
! disable CPU interrupt

	.align	16
_lock:
	cli				! disable interrupt
	ret

!*==============================================================================*
!*					unlock					*
!*==============================================================================*
! PUBLIC void unlock();
! enable CPU interrupt

	.align	16
_unlock:
	sti
	ret

!*==============================================================================*
!*					enable_irq				*
!*==============================================================================*
! PUBLIC void enable_irq(unsigned irq)
! enable an interrupt request line by clearing an 8259 bit.
! equivalent C code for hook->irq < 8:
! 	out_byte(INT_CTLMASK,in_byte(INT_CTLMASK) & ~(1<<ieq);
	
	.align	16
_enable_irq:
	mov	ecx, 4(esp)		! irq
	pushf
	cli
	movb	ah, ~1
	rolb	ah, cl			! ah = ~(1 << (irq % 8))
	cmpb	cl, 8
	jae	enable_8		! enable IRQs of 8 or higher on slave 8259
enable_0:
	inb	INT_CTLMASK
	andb	al,ah
	outb	INT_CTLMASK		! clear bit with master 8259
	popf
	ret
	.align	4
enable_8:
	inb	INT2_CTLMASK
	andb	al,ah
	outb	INT2_CTLMASK		! clear bit with slave 8259
	popf
	ret


!*==============================================================================*
!*					disable_irq				*
!*==============================================================================*
! PUBLIC int disable_irq(unsigned irq)
! disable an interrupt request line by setting an 8259 bit.
! equivalent C code for irq < 8:
!	ir_actids[hook->irq] |= hook->id;
!	outb(INT_CTLMASK,inb(INT_CTLMAST) | (1 << irq));
! returns true if the interrupt was not already disabled.

	.align	16
_disable_irq:
	mov	ecx, 4(esp)		! irq
	pushf
	cli
	movb	ah, 1
	rolb	ah, cl			! ah = ( 1 << (irq % 8))
	cmpb	cl, 8
	jae	disable_8		! disable IRQSs of 8 and above with slabe 8259
disable_0:
	inb	INT_CTLMASK
	testb	al,ah
	jnz	dis_already		! is it already disabled?
	orb	al,ah
	outb	INT_CTLMASK		! set bit with master 8259
	popf
	mov	eax, 1			! disable this function
	ret
disable_8:
	inb	INT2_CTLMASK
	testb	al,ah
	jnz	dis_already		! is it already disabled?
	orb	al,ah
	outb	INT2_CTLMASK		! set bit with slave 8259
	popf
	mov	eax,1			! disable this function
	ret
dis_already:
	popf
	xor	eax,eax			! already disable
	ret

!*==============================================================================*
!*					phys_copy				*
!*==============================================================================*
! PUBLIC void phys_copy(phys_bytes source,phys_byte destination,phys_bytes bytecount);
! copy block of physical memory
!
PC_ARGS =		4 + 4 + 4 + 4	! 4 + 4 + 4
!			es edi esi eip	 src dst len

	.align	16
_phys_copy:
	cld
	push	esi
	push	edi
	push	es

	mov	eax,FLAT_DS_SELECTOR
	mov	es,ax
	
	mov	esi,PC_ARGS(esp)
	mov	edi,PC_ARGS+4(esp)
	mov	eax,PC_ARGS+4+4(esp)

	cmp	eax,10			! avoid the overhead of bordering small counts
	jb	pc_small
	mov	ecx,esi			! it is desirable to match the source and the target as well
	neg	ecx
	and	ecx,3			! count to match
	sub	eax,ecx
	rep
   eseg	movsb
	mov	ecx,eax
	shr	ecx,2			! dword count
	rep
   eseg movs
	and	eax,3
pc_small:
	xchg	ecx,eax			! rest
	rep
   eseg movsb

	pop	es
	pop	edi
	pop	esi
	ret


!*==============================================================================*
!*					mem_rdw					*
!*==============================================================================*
! PUBLIC u16_t mem_rdw(U16_t segment,u16_t *offset);
! loads and returns words with remote pointer segments and offsets

	.align	16
_mem_rdw:
	mov	cx,ds
	mov	ds,4(esp)		! segment
	mov	eax,4+4(esp)		! offset
	movz	eax,(eax)		! word to return
	mov	ds,cx
	ret

!*==============================================================================*
!*					reset					*
!*==============================================================================*
! PUBLIC void reset();
! reset the system by loading IDT with offset 0 and interrupting.

_reset:
	lidt	(idt_zero)
	int	3			! anything goes,the 386 will not like it
.sect	.data
idt_zero:	.data 0, 0
.sect	.text

!*==============================================================================*
!*					mem_vid_copy				*
!*==============================================================================*
! PUBLIC void mem_vid_copy(u16 *src,unsigned dst,unsigned count);
!
! copy count characters from kernel memory to video memory.
! src is an ordinary pointer to a word,but dst and count are character (word) based
! video offset and count. if src is null then screen memory is blanked by filling
! it with blank_color.

MVC_ARGS =		4 + 4 + 4 + 4	! 4 + 4 + 4
!		       es  edi esi eip   src dst ct

_mem_vid_copy:
	push	esi
	push	edi
	push	es
	mov	esi,MVC_ARGS(esp)	! souce
	mov	edi,MVC_ARGS+4(esp)	! destination
	mov	edx,MVC_ARGS+4+4(esp)	! count
	mov	es,(_vid_seg)		! destination is video segment
	cld				! make sure direction is up
mvc_loop:
	and	edi,(_vid_mask)		! wrap address
	mov	ecx,edx			! one chunk to copy
	mov	eax,(_vid_size)
	sub	eax,edi
	cmp	ecx,eax
	jbe	0f
	mov	ecx,eax			! ecx = min(ecx,vid_size - edi)
0:	sub	edx,ecx			! count -= ecx
	shl	edi,1			! byte address
	test	esi,esi			! source == 0 means blank screen
	jz	mvc_blank
mvc_copy:
	rep				! copy word to video memory
    o16	movs
	jmp	mvc_test
mvc_blank:
	mov	eax,(_blank_color)	! ax = character to blank
	rep
    o16	stos				! copy blank to video memory
	!jmp	mvc_test
mvc_test:
	shr	edi,1			! address of word
	test	edx,edx
	jnz	mvc_loop
mvc_done:
	pop	es
	pop	edi
	pop	esi
	ret

!*==============================================================================*
!*					vid_vid_copy				*
!*==============================================================================*
! PUBLIC void vid_vid_copy(unsigned src,unsigned dst,unsigned count);
!
! copy count characters from video memory to video memory.
! handle overlap.used for scrolling,line or character insertion and deletion.
! src,dst and count are character (word) based video offsets and count.

VVC_ARGS	= 	4 + 4 + 4 + 4		! 4 + 4 + 4
!		       es  edi esi eip		 src dst ct

_vid_vid_copy:
	push	esi
	push	edi
	push	es
	mov	esi,VVC_ARGS(esp)		! source
	mov	edi,VVC_ARGS+4(esp)		! destination
	mov	edx,VVC_ARGS+4+4(esp)		! count
	mov	es,(_vid_seg)			! use video segment
	cmp	esi,edi
	jb	vvc_down
vvc_up:
	cld					! up direction
vvc_uploop:
	and	esi,(_vid_mask)			! wrap address
	and	edi,(_vid_mask)
	mov	ecx,edx				! chunk to copy
	mov	eax,(_vid_size)
	sub	eax,esi
	cmp	ecx,eax
	jbe	0f
	mov	ecx,eax				! ecx = min(ecx,vid_size - esi)
0:	mov	eax,(_vid_size)
	sub	eax,edi
	cmp	ecx,eax
	jbe	0f
	mov	ecx,eax				! ecx = min(ecx,vid_size - edi)
0:	sub	edx,ecx
	shl	esi,1
	shl	edi,1				! address of byte
	rep
eseg o16 movs					! copy video word
	shr	esi,1
	shr	edi,1				! word address
	test	edx,edx
	jnz	vvc_uploop			! again?
	jmp	vvc_done
vvc_down:
	std					! down direction	
	lea	esi,-1(esi)(edx*1)		! start copying from the beginning
	lea	edi,-1(edi)(edx*1)
vvc_downloop:
	and	esi,(_vid_mask)			! wrap address
	and	edi,(_vid_mask)
	mov	ecx,edx				! chunk to copy
	lea	eax,1(esi)
	cmp	ecx,eax
	jbe	0f
	mov	ecx,eax				! ecx = min(ecx,esi + 1)
0:	lea	eax,1(edi)
	cmp	ecx,eax
	jbe	0f
	mov	ecx,eax				! ecx = min(ecx,esi + 1)
0:	sub	edx,ecx				! count -= ecx
	shl	esi,1
	shl	edi,1				! byte address
	rep
eseg o16 movs
	shr	esi,1
	shr	edi,1				! word address
	test	edx,edx
	jnz	vvc_donwloop			! again?
	cld					! C compiler expects above
	!jmp	vvc_done
vvc_done:
	pop	es
	pop	edi	
	pop	esi
	ret


!*==============================================================================*
!*					level0					*
!*==============================================================================*
! PUBLIC void level0(void (*func)(void))
! call a function at permission level 0.this is allows kernel tasks to do
! things that are only possible at the most privileged CPU level.
!
_level0:
	mov	eax,4(esp)
	mov	(_level0_func),eax
	int	LEVEL0_VECTOR
	ret
