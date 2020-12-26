
#include "mm.h"
#include <minix/com.h>

#define BUF_SIZE		100
PRIVATE int buf_count;
PRIVATE char print_buf[BUF_SIZE];
PRIVATE message putch_msg;

_PROTOTYPE( FORWARD void flush,(void));

/*==============================================================*
 * 				putk				*
 *==============================================================*/
PUBLIC void putk(c)
int c;
{
	if (c==0 || buf_count == BUFSIZE) flush();
	if (c == '\n') putk('\r');
	if (c != 0) print_buf[buf_count++] = c;
}

/*==============================================================*
 * 				flush				*
 *==============================================================*/
PRIVATE void flush()
{
	if (buf_count == 0) return;
	putch_msg.m_type = DEV_WRITE;
	putch_msg.PROC_NR = 0;
	putch_msg.TTY_LINE = 0;
	putch_msg.ADDRESS = print_buf;
	putch_msg.COUNT = buf_count;
	sendrec(TTY,&putch_msg);
	buf_count = 0;
}
