
#include "kernel.h"
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/keymap.h>
#include "tty.h"
#include "keymaps/us-std.src"


#define KEYBD			0x60
#define KB_COMMAND		0x64
#define KB_CATE_A20		0x02
#define KB_PAUSE_OUTPUT		0xF0
#define KB_RESET		0x01
#define KB_STATUS		0x64
#define KB_ACK			0xFA
#define KB_BUSY			0x02
#define LED_CODE		0xED
#define MAX_KB_ACK_RETRIES	0x1000
#define MXX_KB_BUSY_RETRIES	0x1000
#define KBIT			0x80

#define ESC_SCAN		1
#define SLASH_SCAN		53
#define HOME_SCAN		71
#define DEL_SCAN		83
#define CONSOLE			0
#define MEMCHECK_ADR		0x472
#define MEMCHECK_MAG		0x1234

#define kb_addr()		(&kb_lines[0])
#define KBN_IN_BYTES		32

PRIVATE int alt1;
PRIVATE int alt2;
PRIVATE int capslock;
PRIVATE int esc;
PRIVATE int control;
PRIVATE int caps_off;
PRIVATE int numlock;
PRIVATE int num_off;
PRIVATE int slock;
PRIVATE int slock_off;
PRIVATE int shift;

PRIVATE char numpad_map[] = {
	'H','Y','A','B','D','C','V','U','G','S','T','@'};

struct kb_s{
	char *ihead;
	char *inhead;
	int icount;
	char ibuf[KB_IN_BYTES];
};

PRIVATE struct kb_s kb_lines[NR_CONS];

FORWARD _PROTOTYPE(int kb_ack,(void));
FORWARD _PROTOTYPE(int kb_wait,(void));
FORWARD _PROTOTYPE(int func_key,(int scode));
FORWARD _PROTOTYPE(int scan_keyboard,(void));
FORWARD _PROTOTYPE(unsigned make_break,(int scode));
FORWARD _PROTOTYPE(void set_leds,(void));
FORWARD _PROTOTYPE(int kbd_hw_int,(int irq));
FORWARD _PROTOTYPE(void kb_read,(struct tty *tp));
FORWARD _PROTOTYPE(unsigned map_key,(int scode));

/*==============================================================================*
 * 					map_key0				*
 *==============================================================================*/
#define map_key0(scode)		\
	((unsigned) keymap[(scode) * MAP_COLS])

/*==============================================================================*
 * 					map_key					*
 *==============================================================================*/
PRIVATE unsigned map_key(scode)
int scode;
{
	int caps,column;
	u16_t *keyrow;

	if (scode == SLASH_SCAN && esc) return '/';

	keyrow = &keymap[scode * MAP_COLS];
	
	caps = shift;
	if (numlock && HOME_SCAN <= scode && scode <= DEL_SCAN) caps = !caps;
	if (capslock && (keyrow[0] & HASCAPS)) caps =  !caps;

	if (alt1 || alt2){
		column = 2;
		if (control || alt2) column = 3;
		if (caps) column = 4;
	} else {
		column = 0;
		if (caps) column = 1;
		if (control) column = 5;
	}
	return keyrow[column] & ~HASCAPS;
}

/*==============================================================================*
 * 					kbd_hw_int				*
 *==============================================================================*/
PRIVATE int kbd_hw_int(irq)
int irq;
{
	int code;
	unsigned km;
	register struct kb_s *kb;

	code = scan_keyboard();

	if (code & 0200){
		km = map_key0(code & 0177);
		if (km != CTRL && km != SHIFT && km != ALT && km != CALOCK
				&& km != NLOCK && km != SLOCK && km != EXTKEY)
			return 1;
	}

	kb = kb_addr();
	if (kb->icount < KB_IN_BYTES){
		*kb->inhead++ = code;
		if (kb->ihead == kb->inbuf + KB_IN_BYTES) kb->ihead = kb->ibuf;
		kb->icount++;
		tty_table[current].tty_events = 1;
		force_timeout();
	}
	return 1;
}

/*==============================================================================*
 * 					kb_read					*
 *==============================================================================*/
PRIVATE void kb_read(tp)
tty_t *tp;
{
	struct kb_s *kb;
	char buf[3];
	int scode;
	unsigned ch;

	kb = kb_addr();
	tp = &tty_table[current];

	while(kb->icount > 0){
		scode = *kb->itail++;
		if (kb->itail == kb->ibuf + KB_IN_BYTES) kb->itail = kb->ibuf;
		lock();
		kb->icount--;
		unlock();

		if (func_key(scode)) continue;

		ch = make_break(scode);
		
		if (ch <= 0xFF){
			buf[0] = ch;
			(void) in_process(tp,buf,1);
		}else
		if (HOME <= ch && ch <= INSRT){
			buf[0] = ESC;
			buf[1] = '[';
			buf[2] = numpad_map[ch - HOME];
			(void) in_process(tp,buf,3);
		}else
		if (ch == ALEFT){
			select_console(current - 1);
		}else
		if (ch == ARIGHT){
			select_console(current + 1);
		}else
		if (AF1 <= ch && ch <= AF12){
			select_console(ch - AF1);
		}
	}
}

/*==============================================================================*
 * 					make_break				*
 *==============================================================================*/
PRIVATE unsigned make_break(scode)
int scode;
{
	int ch,make;
	static int CAD_count = 0;

	if (control && (alt1 || alt2) && scode == DEL_SCAN)
	{
		if (++CAD_count == 3) wreboot(RBT_HALT);
		cause_sig(INIT_PROC_NR,SIGABRT);
		return -1;
	}

	make = (scode & 0200 ? 0 : 1);

	ch = map_key(scode & 0177);

	switch(ch){
		case CTRL:
			control = make;
			ch = -1;
			break;
		case SHIFT:
			shift = make;
			ch = -1;
			break;
		case ALT:
			if (make){
				if (esc) alt2 = 1; else alt1 = 1;
			}else{
				alt1 = alt2 = 0;
			}
			ch = -1;
			break;
		case CALOCK:
			if (make && caps_off){
				capslock = 1 - capslock;
				set_leds();
			}
			caps_off = 1 - make;
			ch = -1;
			break;
		case NLOCK:
			if (make && num_off){
				numlock = 1 - numlock;
				set_leds();
			}
			num_off = 1 - make;
			ch = -1;
			break;
		case SLOCK:
			if (make && slock_off){
				slock = 1 - slock;
				set_leds();
			}
			slock_off = 1 - make;
			ch = -1;
			break;
		case EXTKEY:
			esc = 1;
			return (-1);
		default:
			if (!make) ch = -1;
	}
	esc = 0;
	return (ch);
}

/*==============================================================================*
 * 					set_leds				*
 *==============================================================================*/
PRIVATE void set_leds()
{
	unsigned leds;
	if (!pc_at) return;

	leds = (slock << 0) | (numlock << 1) | (capslock << 2);

	kb_wait();
	out_byte(KEYBD,LED_CODE);
	kb_ack();

	kb_wait();
	out_byte(KEYBD,leds);
	kb_ack();
}

/*==============================================================================*
 * 					kb_wait					*
 *==============================================================================*/
PRIVATE int kb_wait()
{
	int retries;
	
	retries = MAX_KB_BUSY_RETRIES + 1;
	while (--retries != 0 && in_byte(KB_STATUS) & KB_BUSY)
		;
	return (retries);
}

/*==============================================================================*
 * 					kb_ack					*
 *==============================================================================*/
PRIVATE int kb_ack()
{
	int retries;
	retries = MAX_KB_ACK_RETRIES + 1;
	while (--retries != 0 && in_byte(KEYBD) != KB_ACK)
		;
	return (retries);
}

/*==============================================================================*
 * 					kb_init					*
 *==============================================================================*/
PUBLIC void kb_init(tp)
tty_t *tp;
{
	register struct kb_s *kb;
	tp->tty_devread = kb_read;
	kb = kb_addr();
	kb->inhead = kb->itail = kb->ibuf;

	caps_off = 1;
	num_off = 1;
	slock_off = 1;
	esc = 0;

	set_leds();
	scan_keyboard();
	put_irq_handler(KEYBOARD_IRQ,kbd_hw_int);
	enable_irq(KEYBOARD_IRQ);
}

/*==============================================================================*
 * 					kbd_loadmap				*
 *==============================================================================*/
PUBLIC int kbd_loadmap(user_phys)
phys_bytes user_phys;
{
	phys_copy(user_phys,vir2phys(keymap),(phys_bytes) sizeof(keymap));
	return (OK);
}
/*==============================================================================*
 * 					func_key				*
 *==============================================================================*/
PRIVATE int func_key(scode)
int scode;
{
	unsigned code;
	code = map_key0(scode);
	if (code < F1 || code > F12) return(FALSE);
	switch(map_key(scode)){
		case F1:	p_dmp();	break;
		case F2:	map_dmp();	break;
		case F3:	toggle_scroll(); break;
		case CF7:	sigchar(&tty_table[CONSOLE],SIGQUIT); break;
		case CF8:	sigchar(&tty_table[CONSOLE],SIGINT);  break;
		case CF9:	sigchar(&tty_table[CONSOLE],SIGKILL); break;
		default:	return (FALSE);
	}
	return (TRUE);
}
/*==============================================================================*
 * 					scan_keyboard				*
 *==============================================================================*/
PRIVATE int scan_keyboard()
{
	int code;
	int val;

	code = in_byte(KEYBD);
	cal = in_byte(PORT_B);
	out_byte(PORT_B,val | KBIT);
	out_byte(PORT_B,val);
	return code;
}
/*==============================================================================*
 * 					wreboot					*
 *==============================================================================*/
PUBLIC void wreboot(how)
int how;
{
	int quiet,code;
	static u16_t magic = MEMCHECK_MAG;
	struct tasktab *ttp;

	out_byte(INT_CTLMASK,~0);

	cons_stop();
	floppy_stop();
	clock_stop();

	if (how == RBT_HALT){
		printf("System Halted\n");
		if (!mon_return) how = RBT_PANIC;
	}
	if (how == RBT_PANIC){
		printf("Hit ESC to reboot,F-keys for debug dumps\n");

		(void) scan_keyboard();
		quiet = scan_keyboard();
		for(;;){
			milli_delay(100);
			code = scan_keyboard();
			if (code != quiet){
				if (code == ESC_SCAN) break;
				(void) func_key(code);
				quiet = scan_keyboard();
			}
		}
		how = RBT_REBOOT;
	}
	if (how == RBT_REBOOT) printf("Rebooting\n");
	if (mon_return && how != RBT_RESET){
		intr_init(0);
		out_byte(INT_CTLMASK,0);
		out_byte(INT2_CTLMASK,0);

		if (how == RBT_HALT){
			reboot_code = vir2phys("");
		}else
		if (how == RBT_REBOOT){
			reboot_code = vir2phys("delay;boot");
		}
		level0(monitor);
	}
	phys_copy(vir2phys(&magic),(phys_bytes) MEMCHECK_ADR,
			(phys_bytes) sizeof(magic));
	if (protected_mode){
		kb_wait();
		out_byte(KB_COMMAND,KB_PULSE_OUTPUT |
				 (0x0F & ~(KB_GATE_A20 | KB_RESET)));
		milli_delay(10);

		printf("Hard reset...\n");
		milli_delay(250);
	}
	level0(reset);
}

