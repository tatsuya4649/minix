// this file contains routines for initializing the 8259 interrupt controller.
// * get_irq_handler : interrupt handler address
// * put_irq_handler : registering at interrupt handler
// * intr_init : interrupt controller initialization
//
//

#include "kernel.h"
#define ICW1_AT			0x11		// requires trigger,cascade,ICW4 at edge
#define ICW1_PC			0x13		// requires trigger,not cascade,ICW4 at ede
#define ICW1_PS			0x19		// requires trigger,cascade,ICW4 ar level
#define ICW4_AT			0x01		// non-SFNM,no buffer,normal EOI,8086
#define ICW4_PC			0x09		// non-SFNM,buffer,normal EOI,8086

FORWARD  _PROTOTYPE(int spurious_irq,(int irq));

#define set_vec(nr,addr)	((void)0)	// protected mode cludge

/*======================================================================================*
 * 					intr_init					*
 *======================================================================================*/
PUBLIC void intr_init(mine)
int mine;
{
	// initialize 8259 and finally disable all interrupts.
	// this is done only in protected mode,not in real mode 8259,instead using the BIOS
	// location. the B flag is set when programming the 8259 for minix or resetting it
	// as needed by the BIOS.
	int i;
	lock();

	// AT and new type PS/2 have two interrupt controllers, one is the master and one is
	// the slave in IRQ 2 (PCs with only one contoller must operate in real mode,
	// so don't worry it's okay.)
	out_byte(INT_CTL,ps_mca ? ICW1_PS : ICW1_AT);
	out_byte(INT_CTLMASK, mine ? IRQ0_VECTOR : BIOS_IRQ0_VEC);	// ICW2 for master
	out_byte(INT_CTLMASK, (1<<CASCADE_IRQ));			// ICW3 show slave
	out_byte(INT_CTLMASK, ICW4_AT);
	out_byte(INT_CTLMASK, ~(1<<CASCADE_IRQ));			// IRQ 0-7 mask
	out_byte(INT2_CTL,ps_mca ? ICW1_PS : ICW1_AT);
	out_byte(INT2_CTLMASK, mine ? IRQ8_VECTOR : BIOS_IRQ8_VEC);	// ICW2 for slave
	out_byte(INT2_CTLMASK, CASCADE_IRQ);				// ICW3 is slave nr
	out_byte(INT2_CTLMASK, ICW4_AT);
	out_byte(INT2_CTLMASK, ~0);					// IRQ 0-15 mask

	// initialize table of interrupt handler
	for (i=0;i<NR_IRQ_VECTOR;i++) irq_table[i] = spurious_irq;
}

/*======================================================================================*
 * 					spurious_irq					*
 *======================================================================================*/
PUBLIC int spurious_irq(irq)
int irq;
{
	// default interrupt handler.generate many message
	if (irq<0 || irq>=NR_IRQ_VECTORS)
		panic("invalid call to spurious_irq",irq);
	printf("spurious irq %d\n",irq);
	return 1;	// re-enable interrupts
}

/*======================================================================================*
 * 					put_irq_handler					*
 *======================================================================================*/
PUBLIC void put_irq_handler(irq,handler)
int irq;
irq_handler_t handler
{
	// register interrupt handler
	if (irq < 0 || irq >= NR_IRQ_VECTORS)
		panic("invalid call to put_irq_handler",irq);
	if (irq_table[irq] == handler)
		return;			// futher initialization

	if (irq_table[irq] != spurious_irq)
		panic("attempt to register second irq handler for irq",irq);

	disable_irq(irq);
	if (!protected_mode) set_vec(BIOS_VECTOR(irq),irq_vec[irq]);
	irq_table[irq] = handler;
	irq_use |= 1 << irq;
}


