


#include "kernel.h"
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/keymap.h>
#include "tty.h"
#include "proc.h"

// adress of tty struct
#define tty_addr(line)	(&tty_table[line])

#define CONS_MINOR		0
#define LOG_MINOR		15
#define RS232_MINOR		16
#define TTYPX_MINOR		128
#define PTYPX_MINOR		192

#define isconsole(tp)		((tp) < tty_addr(NR_CONS))

#define FIRST_TTY		tty_addr(0)
#define END_TTY			tty_addr(sizeof(tty_table)/sizeof(tty_table[0]))

#define tty_active(tp)		((tp)->tty_devread!=NULL)

#if NR_RS_LINES == 0
#define	rs_init(tp)		((void) 0)
#endif
#if NR_PTYS == 0
#define pty_init(tp)		((void) 0)
#define do_pty(tp,mp)		((void) 0)
#endif

FORWARD	_PROTOTYPE(void do_cancel,(tty_t *tp,message *m_ptr));
FORWARD _PROTOTYPE(void do_ioctl,(tty_t *tp,message *m_ptr));
FORWARD _PROTOTYPE(void do_open,(tty_t *tp,message *m_ptr));
FORWARD _PROTOTYPE(void do_close,(tty_t *tp,message *m_ptr));
FORWARD _PROTOTYPE(void do_read,(tty_t *tp,message *m_ptr));
FORWARD _PROTOTYPE(void do_write,(tty_t *tp,message *m_ptr));
FORWARD _PROTOTYPE(void in_transfer,(tty_t *tp));
FORWARD _PROTOTYPE(int	echo,(tty_t *tp,int ch));
FORWARD _PROTOTYPE(void rawecho,(tty_t *tp,int ch));
FORWARD _PROTOTYPE(int back_over,(tty_t *tp));
FORWARD _PROTOTYPE(void reprint,(tty_t *tp));
FORWARD _PROTOTYPE(void dev_ioctl,(tty_t *tp));
FORWARD _PROTOTYPE(void setattr,(tty_t *tp));
FORWARD _PROTOTYPE(void tty_icancel,(tty_t *tp));
FORWARD _PROTOTYPE(void tty_init,(tty_t *tp));
FORWARD _PROTOTYPE(void settimer,(tty_t *tp,int on));

PRIVATE struct termios termios_defaults = {
	TINPUT_DEF,TOUTPUT_DEF,TCTRL_DEF,TLOCAL_DEF,TSPEED_DEF,TSPEED_DEF,
	{
		TEOF_DEF,TEOL_DEF,TERASE_DEF,TINTR_DEF,TKILL_DEF,TMIN_DEF,
		TQUIT_DEF,TTIME_DEF,TSUSP_DEF,TSTART_DEF,TSTOP_DEF,TREPRINT_DEF,
		TLNEXT_DEF,TDISCARD_DEF,
	},
};
PRIVATE struct winsize winsize_defaults;		// all zero

/*==============================================================================*
 * 					tty_task				*
 *==============================================================================*/
PUBLIC void tty_task()
{
	message tty_mess;
	register tty_t *tp;
	unsigned line;

	for(tp=FIRST_TTY;tp<END_TTY;tp++) tty_init(tp);

	printf("Minix %s.%s Copyright 1997 Prentice-Hall,Inc.\n\n",
			OS_RELEASE,OS_VERSION);
	printf("Executing in 32-bit protected mode\n\n");

	while(TRUE){
		for (tp=FIRST_TTY;tp<END_TTY;tp++){
			if (tp->tty_events) handle_events(tp);
		}
		receive(ANY,&tty_mess);

		if (tty_mess.m_type==HARD_INT) continue;

		line = tty_mess.TTY_LINE;
		if ((line - CONS_MINOR) < NR_CONS){
			tp = tty_addr(line - CONS_MINOR);
		}else
		if (line == LOG_MINOR){
			tp = tty_addr(0);
		} else
		if ((line - RS232_MINOR) < NR_RS_LINES){
			tp = tty_addr(line - RS232_MINOR + NR_CONS);
		}else
		if ((line - TTYPX_MINOR) < NR_PTYS){
			tp = tty_addr(line - TTYPX_MINOR + NR_CONS + NR_RS_LINES);
		} else
		if ((line - PTYPX_MINOR) < NR_PTYS){
			tp = tty_addr(line - PTYPX_MINOR + NR_CONS + NR_RS_LINES);
			do_pty(tp,&tty_mess);
			continue;
		}else{
			tp = NULL;
		}

		if (tp == NULL || !tty_active(tp)){
			tty_reply(TASK_REPLY,tty_mess.m_source,
					tty_mess.PROC_NR,ENXIO);
			continue;
		}

		switch(tty_mess.m_type){
			case DEV_READ:	do_read(tp,&tty_mess);		break;
			case DEV_WRITE: do_write(tp,&tty_mess); 	break;
			case DEV_IOCTL: do_ioctl(tp,&tty_mess); 	break;
			case DEV_OPEN:	do_open(tp,&tty_mess);		break;
			case DEV_CLOSE:	do_close(tp,&tty_mess);		break;
			case CANCEL:	do_cancel(tp,&tty_mess);	break;
			default:	tty_reply(TASK_REPLY,tty_mess.m_source,
							tty_mess.PROC_NR,EINVAL);
		}
	}
}

/*==============================================================================*
 * 					do_read					*
 *==============================================================================*/
PRIVATE void do_read(tp,m_ptr)
register tty_t	*tp;
message	*m_ptr;
{
	int	r;

	if (tp->tty_inleft > 0){
		r = EIO;
	}else
	if (m_ptr->COUNT <= 0){
		r = EINVAL;
	}else
	if (numap(m_ptr->PROC_NR,(vir_bytes) m_ptr->ADDRESS,m_ptr->COUNT) == 0){
		r = EFAULT;
	}else{
		tp->tty_inrepcode = TASK_REPLY;
		tp->tty_incaller = m_ptr->m_source;
		tp->tty_inproc = m_ptr->PROC_NR;
		tp->tty_in_vir = (vir_bytes) m_ptr->ADDRESS;
		tp->tty_inleft = m_ptr->COUNT;

		if(!(tp->tty_termios.c_lflag & ICANON)
				&& tp->tty_termios.c_cc[VTIME] > 0){
			if (tp->tty_termios.c_cc[VMIN] == 0){
				lock();
				settime(tp,TRUE);
				tp->tty_min = 1;
				unlock();
			}else{
				if (tp->tty_eotct == 0){
					lock();
					settime(tp,FALSE);
					unlock();
					tp->tty_min = tp->tty_termios.c_cc[VMIN];
				}
			}
		}
		in_transfer(tp);
		handle_events(tp);
		if (tp->tty_inleft == 0) return;
		if (m_ptr->TTY_FLAGS & O_NONBLOCK){
			r = EAGAIN;
			tp->ptty_inleft = tp->tty_incum = 0;
		}else{
			r = SUSPEND;
			tp->tty_inrepcode = REVICE;
		}
	}
	tty_reply(TASK_REPLY,m_ptr->m_source,m_ptr->PROC_NR,r);
}


/*==============================================================================*
 * 					do_write				*
 *==============================================================================*/
PRIVATE void do_write(tp,m_ptr)
register tty_t *tp;
register message *m_ptr;
{
	int r;

	if (tp->tty_outleft > 0){
		r = EIO;
	}else
	if (numap(m_ptr->PROC_NR,(vir_bytes) m_ptr->ADDRESS,m_ptr->COUNT) == 0){
		r = EFAULT;
	} else {
		tp->tty_outrepcode = TASK_REPLY;
		tp->tty_outcaller = m_ptr->m_source;
		tp->tty_outproc = m_ptr->PROC_NR;
		tp->tty_out_vir = (vir_bytes) m_ptr->ADDRESS;
		tp->tty_outleft = m_ptr->COUNT;

		handle_events(tp);
		if (tp->tty_outleft == 0) return;

		if (m_ptr->TTY_FLAGS & O_NONBLOCK){
			r = tp->tty_outcum > 0 ? tp->tty_outcum : EAGAIN;
			tp->tty_outleft = tp->tty_outcum = 0;
		} else {
			r = SUSPEND;
			tp->tty_outrepcode = REVIVE;
		}
	}
	tty_reply(TASK_REPLY,m_ptr->m_source,m_ptr->PROC_NR,r);
}

/*==============================================================================*
 * 					do_ioctl				*
 *==============================================================================*/
PRIVATE void do_ioctl(tp,m_ptr)
register tty_t *tp;
message *m_ptr;
{
	int	r;
	union{
		int i;
	} param;
	phys_bytes user_phys;
	size_t	size;

	switch(m_ptr->TTY_REQUEST){
		case TCGETS:
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			size = sizeof(struct termios);
			break;
		case TCSBRK:
		case TCFLOW:
		case TCFLSH:
		case TIOCGPGRP:
		case TIOCSPGRP:
			size = sizeof(int);
			break;
		case TIOCGWINSZ:
		case TIOCSWINSZ:
			size = sizeof(struct winsize);
			break;
		case KIOCSMAP:
			size = sizeof(keymap_t);
			break;
		case TIOCSFON:
			size = sizeof(u8_t [8192]);
			break;
		case TCDRAIN:
		default:	size = 0;
	}

	if (size != 0){
		user_phys = numap(m_ptr->PROC_NR,(vir_bytes) m_ptr->ADDRESS,size);
		if (user_phys == 0){
			tty_reply(TASK_REPLY,m_ptr->m_source,m_ptr->PROC_NR,EFAULT);
			return;
		}
	}

	r = OK;
	switch(m_ptr->TTY_REQUEST){
		case TCGETS:
			phys_copy(vir2phys(&tp->tty_termios),user_phys,(phys_bytes) size);
			break;
		case TCSETSW:
		case TCSETSF:
		case TCDRAIN:
			if (tp->tty_outleft > 0){
				tp->tty_iocaller = m_ptr->m_source;
				tp->tty_ioproc = m_ptr->PROC_NR;
				tp->tty_ioreq = m_ptr->REQUEST;
				tp->tty_iovir = (vir_bytes) m_ptr->ADDRESS;
				r = SUSPEND;
				break;
			}
			if (m_ptr->TTY_REQUEST == TCDRAIN) break;
			if (m_ptr->TTY_REQUEST == TCSETSF) tty_icancel(tp);
		case TCSETS:
			phys_copy(user_phys,vir2phys(&tp->tty_termios),(phys_bytes) size);
			setattr(tp);
			break;
		case TCFLSH:
			phys_copy(user_phys,vir2phys(&param.i),(phys_bytes) size);
			switch(param,i){
				case TCIFLUSH:	tty_icancel(tp);	break;
				case TCOFLUSH:	(*tp->tty_ocancel)(tp); break;
				case TCIOFLUSH: tty_icancel(tp); (*tp->tty_ocancel)(tp); break;
				default:	r = EINVAL;
			}
			break;

		case TCFLOW:
			phys_copy(user_phys,vir2phys(&param.i),(phys_bytes) size);
			switch(param.i){
				case TCOOFF:
				case TCOON:
					tp->tty_inhibited = (param.i == TCOOFF);
					tp->tty_events = 1;
					break;
				case TCIOFF:
					(*tp->tty_echo)(tp,tp->tty_termios.c_cc[VSTOP]);
					break;
				case TCION:
					(*tp->tty_echo)(tp,tp->tty_termios.c_cc[VSTART]);
					break;
				default:
					r = EINVAL;
			}
			break;
		case TCSBRK:
			if (tp->tty_break != NULL) (*tp->tty_break)(tp);
			break;
		case TIOCGWINSZ:
			phys_copy(vir2phys(&tp->tty_winsize),user_phys,(phys_bytes) size);
			break;
		case TIOCSWINSZ:
			phys_copy(user_phys,vir2phys(&tp->tty_winsize),(phys_bytes) size);
			break;
		case KIOCSMAP:
			if (isconsole(tp)) r = kbd_loadmap(user_phys);
			break;
		case TIOCSFON:
			if (isconsole(tp)) r = con_loadfont(user_phys);
			break;

		case TIOCGPGRP:
		case TIOCSPGRP:
		default:
			r = ENOTTY;
	}
	tty_reply(TASK_REPLY,m_ptr->m_source,m_ptr->PROC_NR,r);
}

/*==============================================================================*
 * 					do_open					*
 *==============================================================================*/
PRIVATE void do_open(tp,m_ptr)
register tty_t *tp;
message *m_ptr;
{
	int r = OK;

	if (m_ptr->TTY_LINE == LOG_MINOR){
		if (m_ptr->COUNT & R_BIT) r = = EACCES;
	}else{
		if (!(m_ptr->COUNT & O_NOCTTY)){
			tp->tty_pgrp = m_ptr->PROC_NR;
			r = 1;
		}
		tp->tty_openct++;
	}
	tty_reply(TASK_REPLY,m_ptr->m_source,m_ptr->PROC_NR,r);
}

/*==============================================================================*
 * 					do_close				*
 *==============================================================================*/
PRIVATE void do_close(tp,m_ptr)
register tty_t *tp;
message *m_ptr;
{
	if (m_ptr->TTY_LINE != LOG_MINOR && --tp->tty_openct == 0){
		tp->tty_pgrp = 0;
		tty_icancel(tp);
		(*tp->tty_ocancel)(tp);
		(*tp->tty_close)(tp);
		tp->tty_termios = termios_defaults;
		tp->tty_winsize = winsize_defaults;
		setattr(tp);
	}
	tty_reply(TASK_REPLY,m_ptr->m_source,m_ptr->PROC_NR,OK);
}

/*==============================================================================*
 * 					do_cancel				*
 *==============================================================================*/
PRIVATE void do_cancel(tp,m_ptr)
register tty_t *tp;
message *m_ptr;
{
	int proc_nr;
	int mode;

	proc_nr = m_ptr->PROC_NR;
	mode = m_ptr->COUNT;
	if ((mode & R_BIT) && tp->tty_inleft != 0 && proc_nr == tp->tty_inproc){
		tty_icancel(tp);
		tp->tty_inleft = tp->tty_incum = 0;
	}
	if ((mode & W_BIT) && tp->tty_outleft != 0 && proc_nr == tp->tty_outproc){
		(*tp->tty_ocancel)(tp);
		tp->tty_outleft = tp->tty_outcum = 0;
	}
	if (tp->tty_ioreq != 0 && proc_nr == tp->tty_ioproc){
		tp->tty_ioreq = 0;
	}
	tp->tty_events = 1;
	tty_reply(TASK_REPLY,m_ptr->m_source,proc_nr,EINTR);
}

/*==============================================================================*
 * 				handle_events					*
 *==============================================================================*/
PUBLIC void handle_events(tp)
tty_t *tp;
{
	char *buf;
	unsigned count;

	do{
		tp->tty_events = 0;
		(*tp->tty_devread)(tp);

		(*tp->tty_devwrite)(tp);

		if(tp->tty_ioreq != 0) dev_ioctl(tp);
	} while(tp->tty_events);

	in_transfer(tp);

	if (tp->tty_incum >= tp->tty_min && tp->tty_inleft > 0){
		tty_reply(tp->tty_inrepcode,tp->tty_incaller,tp->tty_inproc,
				tp->tty_incum);
		tp->tty_inleft = tp->tty_incum = 0;
	}
}

/*==============================================================================*
 * 				in_transfer					*
 *==============================================================================*/
PRIVATE void in_transfer(tp)
register tty_t *tp;
{
	int ch;
	int count;
	phys_bytes buf_phys,user_base;
	char buf[64],*bp;

	if(tp->tty_inleft == 0 || tp->tty_eotct < tp->tty_min) return;
	buf_phys = vir2phys(buf);
	user_base = proc_vir2phys(proc_addr(tp->tty_inproc),0);
	bp = buf;
	while(tp->tty_inleft > 0 && tp->tty_eotct > 0){
		ch = *tp->tty_intail;

		if (!(ch & IN_EOF)){
			*bp = ch & IN_CHAR;
			tp->tty_inleft--;
			if (++bp == bufend(buf)){
				phys_copy(buf_phys,user_base+tp->tty_in_vir,
						(phys_bytes) buflen(buf));
				tp->tty_in_vir += buflen(buf);
				tp->tty_incum += buflen(buf);
				bp = buf;
			}
		}

		if (++tp->tty_intail == bufend(tp->tty_inbuf))
			tp->tty_intail = tp->tty_inbuf;
		tp->tty_incount--;
		if (ch & IN_EOT){
			tp->tty_eotct--;
			if (tp->tty_termios.c_lflag & ICANON) tp->tty_inleft = 0;
		}
	}
	if (bp > buf){
		count = bp - buf;
		phys_copy(buf_phys,user_base + tp->tty_in_vir,(phys_bytes)count);
		tp->tty_in_vir += count;
		tp->tty_incum += count;
	}
	if (tp->tty_inleft == 0){
		tty_reply(tp->tty_inrepcode,tp->tty_incaller,tp->tty_inproc,
				tp->tty_incum);
		tp->tty_inleft = tp->tty_incum = 0;
	}
}

/*==============================================================================*
 * 				in_process					*
 *==============================================================================*/
PUBLIC int in_process(tp,buf,count)
register tty_t *tp;
char *buf;
int count;
{
	int ch,sig,ct;
	int timeset = FALSE;
	static unsigned char csize_mask[] = {0x1F,0x3F,0x7F,0xFF};

	for(ct=0;ct<count;ct++){
		ch = *buf++ & BYTE;

		if (tp->tty_termios.c_iflag & ISTRIP) ch &= 0x7F;

		if (tp->tty_termios.c_lflag & IEXTEN){

			if (tp->tty_escaped){
				tp->tty_escaped = NOT_ESCAPED;
				ch |= IN_ESC;
			}

			if (ch == tp->tty_termios.c_cc[VLNEXT]){
				tp->tty_escaped = ESCAPED;
				rawecho(tp,'^');
				rawecho(tp,'\b');
				continue;
			}
			if (ch == tp->tty_termios.c_cc[VREPRINT]){
				reprint(tp);
				continue;
			}
		}

		if (ch == _POSIX_VDISABLE) ch |= IN_ESC;
		if (ch == '\r'){
			if (tp->tty_termios.c_iflag & IGNCR) continue;
			if (tp->tty_termios.c_iflag & ICRNL) ch = '\n';
		} else
		if (ch == '\n'){
			if (tp->tty_termios.c_iflag & INLCR) ch = '\r';
		}

		if (tp->tty_termios.c_lflag & ICANON){
			if (ch == tp->tty_termios.c_cc[VERASE]){
				(void) back_over(tp);
				if (!(tp->tty_termios.c_lflag & ECHOE)){
					(void) echo(tp,ch);
				}
				continue;
			}
			if (ch == tp->tty_termios.c_cc[VKILL]){
				while(back_over(tp)){}
				if (!(tp->tty_termios.c_lflag & ECHOE)){
					(void) echo(tp,ch);
					if (tp->tty_termios.c_lflag & ECHOK)
						rawecho(tp,'\n');
				}
				continue;
			}

			if (ch == tp->tty_termios.c_cc[VEOF]) ch |= IN_EOT | IN_EOF;

			if (ch == '\n') ch |= IN_EOF;

			if (ch == tp->termios.c_cc[VEOL]) ch |= IN_EOT;
		}

		if (tp->tty_termios.c_iflag & IXON){
			if (ch == tp->tty_termios.c_Cc[VSTOP]){
				tp->tty_inhibited = STOPPED;
				tp->tty_events = 1;
				continue;
			}

			if (tp->tty_inhibited){
				if (ch == tp->tty_termios.c_cc[VSTART]
						|| (tp->tty_termios.c_iflag & IXANY)){
					tp->tty_inhibited = RUNNING;
					tp->tty_events = 1;
					if (ch == tp->tty_termios.c_cc[VSTART])
						continue;
				}
			}
		}
		
		if (tp->tty_termios.c_lflag & ISIG){
			if (ch == tp->tty_termios.c_cc[VINTR]
					|| ch == tp->tty_termios.c_cc[VQUIT]){
				sig = SIGINT;
				if (ch == tp->tty_termios.c_cc[VQUIT]) sig = SIGQUIT;
				sigchar(tp,sig);
				(void) echo(tp,ch);
				continue;
			}
		}

		if (tp->tty_incount == buflen(tp->tty_inbuf)){
			if (tp->tty_termios.c_lflag & ICANON) continue;
			break;
		}

		if (!(tp->tty_termios.c_lflag & ICANON)){
			ch |= IN_EOT;

			if (!timeset && tp->tty_termios.c_cc[VMIN] > 0
					&& tp->tty_termios.c_cc[VTIME] > 0){
				lock();
				settimer(tp,TRUE);
				unlock();
				timeset = TRUE;
			}
		}

		if (tp->tty_termios.c_lflag & (ECHO|ECHONL)) ch = echo(tp,ch);

		*tp->tty_inhead++ = ch;
		if (tp->tty_inhead == bufend(tp->tty_inbuf))
			tp->tty_inhead = tp->tty_inbuf;
		tp->tty_incount++;
		if (ch & IN_EOT) tp->tty_eotct++;

		if (tp->tty_incount == buflen(tp->tty_inbuf)) in_transfer(tp);
	}
	return ct;
}

/*==============================================================================*
 * 					echo					*
 *==============================================================================*/
PRIVATE int echo(tp,ch)
register tty_t *tp;
register int ch;
{
	int len,rp;

	ch &= ~IN_LEN;
	if (!(tp->tty_termios.c_lflag & ECHO)){
		if (ch == ('\n' | IN_EOT) && (tp->tty_termios.c_lflag
					& (ICANON | ECHONL)) == (ICANON | ECHONL))
			(*tp->tty_echo)(tp,'\n');
		return (ch);
	}

	rp = tp->tty-incount = 0 ? FALSE : tp->tty_reprint;

	if ((ch & IN_CHAR) < ' '){
		switch(ch & (IN_ESC | IN_EOF | IN_EOT | IN_CHAR)){
			case '\t':
				len = 0;
				do{
					(*tp->tty_echo)(tp,' ');
					len++;
				}while(len < TAB_SIZE && (tp->tty_position & TAB_MASK) != 0);
				break;
			case '\r' | IN_EOT:
			case '\n' | IN_EOT:
				(*tp->tty_echo)(tp,ch & IN_CAHR);
				len = 0;
				break;
			default:
				(*tp->tty_echo)(tp,'^');
				(*tp->tty_echo)(tp,'@' + (ch & IN_CHAR));
				len = 2;
		}
	} else
	if ((ch & IN_CHAR) == '\177'){
		(*tp->tty_echo)(tp,'^');
		(*tp->tty_echo)(tp,'?');
		len = 2;
	}else{
		(*tp->tty_echo)(tp,ch & IN_CHAR);
		len = 1;
	}
	if (ch & IN_EOF) while(len > 0){ (*tp->tty_echo)(tp,'\b'); len--; }

	tp->tty_reprint = rp;
	return (ch | (len << IN_LSHIFT));
}


/*==============================================================================*
 * 					rawecho					*
 *==============================================================================*/
PRIVATE void rawecho(tp,ch)
register tty_t *tp;
int ch;
{
	int rp = tp->tty_reprint;
	if (tp->tty_termios.c_lflag & ECHO) (*tp->tty_echo)(tp,ch);
	tp->tty_reprint = rp;
}

/*==============================================================================*
 * 					back_over				*
 *==============================================================================*/
PRIVATE int back_over(tp)
register tty_t *tp;
{
	u16_t *head;
	int len;

	if (tp->tty_incount == 0) return (0);
	head = tp->tty_inhead;
	if (head == tp->tty_inbuf) head = bufend(tp->tty_inbuf);
	if (*--head & IN_EOT) return (0);

	if (tp->tty_reprint) reprint(tp);
	tp->tty_inhead = head;
	tp->tty_incount--;
	if (tp->tty_termios.c_lflag & ECHOE){
		len = (*head & IN_LEN) >> IN_LSHIFT;
		while(len > 0){
			rawecho(tp,'\b');
			rawecho(tp,' ');
			rawecho(tp,'\b');
			len--;
		}
	}
	return (1);
}

/*==============================================================================*
 * 					reprint					*
 *==============================================================================*/
PRIVATE void reprint(tp)
register tty_t *tp;
{
	int count;
	u16_t *head;

	tp->tty_reprint = FALSE;

	head = tp->tty_inhead;
	count = tp->tty_incount;
	while(count > 0){
		if (head == tp->tty_inbuf) head = bufend(tp->tty_inbuf);
		if (head[-1] & IN_EOT) break;
		head--;
		count--;
	}
	if (count == tp->tty_incount) return;

	(void) echo(tp,tp->tty_termios.c_cc[VREPRINT] | IN_ESC);
	rawecho(tp,'\r');
	rawecho(tp,'\n');

	do{
		if (head == bufend(tp->tty_inbuf)) head = tp->tty_inbuf;
		*head = echo(tp,*head);
		head++;
		count++;
	} while( count < tp->tty_incount);
}


/*==============================================================================*
 * 				out_process					*
 *==============================================================================*/
PUBLIC void out_process(tp,bstart,bpos,bend,icount,ocount)
tty_t *tp;
char *bstart,*bpos,*bend;
int *icount;
int *ocount;
{
	int tablen;
	int ict = *icount;
	int oct = *ocount;
	int pos = tp->tty_position;
	while(ict > 0){
		switch(*bpos){
			case '\7':
				break;
			case '\b':
				pos--;
				break;
			case '\r':
				pos = 0;
				break;
			case '\n':
				if ((tp->tty_termios.c_oflag & (OPOST | ONLCR))
						== (OPOST | ONLCR)){
					if (oct >= 2){
						*bpos = '\r';
						if (++bpos == bend) bpos = bstart;
						*bpos = '\n';
						pos = 0;
						ict--;
						oct -= 2;
					}
					goto out_done;
				}
				break;
			case '\t':
				tablen = TAB_SIZE - (pos & TAB_MASK);
				if ((tp->tty_termios.c_oflag & (OPOST | XTABS))
						== (OPOST | XTABS)){
					if (oct >= tablen){
						pos += tablen;
						ict--;
						oct -= tablen;
						do{
							*bpos = ' ';
							if (++bpos == bend) bpos = bstart;
						} while(--tablen != 0);
					}
					goto out_done;
				}
				pos += tablen;
				break;
			default:
				pos++;
		}
		if (++bpos == bend) bpos = bstart;
		ict--;
		oct--;
	}
out_done:
	tp->tty_position = pos & TAB_MASK;
	
	*icount -= ict;
	*ocount -= oct;
}

/*==============================================================================*
 * 				dev_ioctl					*
 *==============================================================================*/
PRIVATE void dev_ioctl(tp)
tty_t *tp;
{
	phys_bytes user_phys;

	if (tp->tty_outleft > 0) return;
	if (tp->tty_ioreq != TCDRAIN){
		if (tp->tty_ioreq == TCSETSF) tty_icencel(tp);
		user_phys = proc_vir2phys(proc_addr(tp->tty_ioproc),tp->tty_iovir);
		phys_copy(user_phys,vir2phys(&tp->tty_temios),
				(phys_bytes) sizeof(tp->tty_termios));
		setattr(tp);
	}
	tp->tty_ioreq = 0;
	tty_reply(REVICE,tp->tty_iocaller,tp->tty_ioproc,OK);
}

/*==============================================================================*
 * 				setattr						*
 *==============================================================================*/
PRIVATE void setattr(tp)
tty_t *tp;
{
	u16_t *inp;
	int count;

	if (!(tp->tty_termios.c_lflag & ICANON)){
		count = tp->tty_eotct = tp->tty_incount;
		inp = tp->tty_intail;
		while(count > 0){
			*inp |= IN_EOT;
			if (++inp == bufend(tp->tty_inbuf)) inp = tp->tty_inbuf;
			--count;
		}
	}

	lock();
	settimer(tp,FALSE);
	unlock();
	if (tp->tty_termios.c_lflag & ICANON){
		tp->tty_min = 1;
	} else{
		tp->tty_min = tp->ty_termios.c_cc[VMIN];
		if (tp->tty_min == 0 && tp->tty_termios.c_cc[VTIME] > 0)
			tp->tty_min = 1;
	}

	if (!(tp->tty_termios.c_iflag & IXON)){
		tp->tty_inhibited = RUNNING;
		tp->tty_events = 1;
	}
	if (tp->tty_termios.c_ospeed = B0) sigchar(tp,SIGHUP);

	(*tp->tty_ioctl)(tp);
}

/*==============================================================================*
 * 				tty_reply					*
 *==============================================================================*/
PUBLIC void tty_reply(code,replyee,proc_nr,status)
int code;
int replyee;
int proc_nr;
int status;
{
	message tty_mess;

	tty_mess.m_type = code;
	tty_mess.REP_PROC_NR = proc_nr;
	tty_mess.REP_STATUS = status;
	if ((status = send(replyee,&tty_mess)) != OK)
		panic("tty_reply failed,status\n",status);
}

/*==============================================================================*
 * 				sigchar						*
 *==============================================================================*/
PUBLIC void sigchar(tp,sig)
register tty_t *tp;
int sig;
{
	if (tp->tty_pgrp != 0) cause_sig(tp->tty_pgrp,sig);

	if (!(tp->tty_termios.c_lflag & NOFLSH)){
		tp->tty_incount = tp->tty_eotct = 0;
		tp->tty_intail = tp->tty_inhead;
		(*tp->tty_ocancel)(tp);
		tp->tty_inhibited = RUNNING;
		tp->tty_events = 1;
	}
}

/*==============================================================================*
 * 				tty_icancel					*
 *==============================================================================*/
PRIVATE void tty_icancel(tp)
register tty_t *tp;
{
	tp->tty_incount = tp->tty_eotct = 0;
	tp->tty_intail = tp->tty_inhead;
	(*tp->tty_icancel)(tp);
}

/*==============================================================================*
 * 				tty_init					*
 *==============================================================================*/
PRIVATE void tty_init(tp)
tty_t *tp;
{
	tp->tty_intail = tp->tty_inhead = tp->tty_inbuf;
	tp->tty_min = 1;
	tp->tty_termios = termios_defaults;
	tp->tty_icancel = tp->tty_ocancel = tp->tty_ioctl = tp->tty_close
		= tty_devnop;

	if (tp < tty_addr(NR_CONS)){
		scr_init(tp);
	}else
	if (tp < tty_addr(NR_CONS + NR_RS_LINES)){
		rs_init(tp);
	}else{
		pty_init(tp);
	}
}

/*==============================================================================*
 * 				tty_wakeup					*
 *==============================================================================*/
PUBLIC void tty_wakeup(now)
clock_t now;
{
	tty_t *tp;
	tty_timeout = TIME_NEVER;
	while ((tp = tty_timelist) != NULL){
		if (tp->tty_time > now){
			tty_timeout = tp->tty_time;
			break;
		}
		tp->tty_min = 0;
		
		tp->tty_events = 1;
		tty_timelist = tp->tty_timenext;
	}

	interrupt(TTY);
}

/*==============================================================================*
 * 				settimer					*
 *==============================================================================*/
PRIVATE void settimer(tp,on)
tty_t *tp;
int on;
{
	tty_t **ptp;

	for (ptp = &tty_timelist; *ptp != NULL; ptp = &(*ptp)->tty_timenext){
		if (tp==*ptp){
			*ptp = tp->tty_timenext;
			break;
		}
	}
	if (!on) return;

	tp->tty_time = get_uptime() + tp->tty_termios.c_cc[VTIME] * (HZ/10);

	for(ptp = &tty_timelist; *ptp != NULL;ptp = &(*ptp)->tty_timenext){
		if (tp->tty_time <= (*ptp)->tty_time) break;
	}
	tp->tty_timenext = *ptp;
	*ptp = tp;
	if (tp->tty_time < tty_timeout) tty_timeout = tp->tty_time;
}

/*==============================================================================*
 * 				tty_devnop					*
 *==============================================================================*/
PUBLIC void tty_devnop(tp)
tty_t *tp;
{

}
