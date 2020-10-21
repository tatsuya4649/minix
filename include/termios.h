// termios.h header define constant,macro,and function prototypes
// used to control terminal I/O devices.
//
// in other words, used to control terminal mode.
// 

#ifndef _TERMIOS_H
#define _TERMIOS_H

typedef unsigned short tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int  speed_t;

#define NCCS		20	// spare space for cc_c array size,extension

// first terminal control structure
struct termios {
	tcflag_t	c_iflag;	// input mode
	tcflag_t	c_oflag;	// output mode
	tcflag_t	c_cflag;	// control mode
	tcflag_t	c_lflag;	// local mode
	speed_t		c_ispeed;	// input speed
	speed_t		c_ospeed;	// output speed
	cc_t		c_cc[NCCS];	// control character
};

// termios c_iflag bitmap value
#define BRKINT		0x0001		// signal interrupt at break
#define ICRNL		0x0002		// when input, map CR to NL
#define IGNBRK		0x0004		// ignore break
#define IGNCR		0x0008		// ignore CR
#define IGNPAR		0x0010		// ignore characters with parity error
#define INLCR		0x0020		// when input, map NL to CR
#define INPCK		0x0040		// enable iput parity check
#define ISTRIP		0x0080		// mask off the 8th bit
#define IXOFF		0x0100		// enable start/stop input control
#define IXON		0x0200		// enable start/stop output control
#define PARMRK		0x0400		// mark input queue parity error

// termios c_oflag bitmap value
#define OPOST		0x0001		// execute output processing

// termios c_cflag bitmap value
#define CLOCAL		0x0001		// ignore modem status line
#define CREAD		0x0002		// enable reciever
#define CSIZE		0x0003		// number of bits per character
#define CS5		0x0000		// if CSIZE is CS5, 5 bit
#define CS6		0x0004		// if CSIZE is CS6, 6 bit
#define CS7		0x0008		// if CSIZE is CS7, 7 bit
#define CS8		0x000C		// if CSIZE is CS8, 8 bit
#define CSTOPB		0x0010		// send 2 stop bits when on, 1 when off
#define HUPCL		0x0020		// hang at last close
#define PARENB		0x0040		// enable output parity
#define PARODD		0x0080		// use odd parity when on,even parity when off

// termios c_lflag bitmap value
#define ECHO		0x0001		// enable echo of input characters
#define ECHOE		0x0002		// error ERASE as backspace
#define ECHOK		0x0004		// echo KILL
#define ECHONL		0x0008		// echo NL
#define ICANON		0x0010		// legitimate input (disable erase and kill)
#define IEXTEN		0x0020		// disable expanded functions
#define ISIG		0x0040		// disable signal
#define NOFLSH		0x0080		// after interrupt or quit,disable flush
#define TOSTOP		0x0100		// send SIGTTOU

// index for c_cc array. default value in parentheses
#define VEOF		0		// cc_c[VFOF] = EOF (^D)
#define VEOL		1		// cc_c[VEOL] = EOL (undef)
#define VERASE		2		// cc_c[VERASE] = ERASE (^H)
#define VINTR		3		// cc_c[VINTR] = INTR (DEL)
#define VKILL		4		// cc_c[VKILL] = KILL (^U)
#define VMIN		5		// cc_c[VMIN] = minimum value of timer
#define VQUIT		6		// cc_c[VQUIT] = QUIT (^\)
#define VTIME		7		// cc_c[VTIME] = TIME value of timer
#define VSUSP		8		// cc_c[VSISP] = SUSP (^Z, ignored)
#define VSTART		9		// cc_c[VSTART] = START (^S)
#define VSTOP		10 		// cc_c[VSTOP] = STOP (^O)

#define _POSIX_VDISABLE	(cc_t)0xFF	// you can't type this character on a regular keyboard,but you can type 0XFF on some language-specific keyboards. cc_t should be short,as all 256 are likely to be used.

// baudrate setting value
#define B0		0x0000		// disconnect the line
#define B50		0x1000		// 50    baud
#define B75		0x2000		// 75    baud
#define B110		0x3000		// 110   baud
#define B134		0x4000		// 134   baud
#define B150		0x5000		// 150   baud
#define B200		0x6000		// 200   baud
#define B300		0x7000		// 300   baud
#define B600		0x8000		// 600   baud
#define B1200		0x9000		// 1200  baud
#define B1800		0xA000		// 1800  baud
#define B2400		0xB000		// 2400  baud
#define B4800		0xC000		// 4800  baud
#define B9600		0xD000		// 9600	 baud
#define B19200		0xE000		// 19200 baud
#define B38400		0xF000		// 38400 baud

// adding action of tcsetattr()
#define TCSANOW		1		// change take effect immediately
#define TCSADRAIN	2		// change take effect after output
#define TCSAFLUSH	3		// flush the input after finishing the output

// Queue_selector value of tcflush() 
#define TCIFLUSH	1		// flush cumulative input data
#define TCOFLUSH	2		// flush cumulative output data
#define TCIOFLUSH	3		// flush cumulative input/output data

// action value of tcflow()
#define TCOOFF		1		// pause output
#define TCOON		2		// restart suspended output
#define TCIOFF		3		// send STOP charactor
#define TCION		4		// send START charactor

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE( int tcsendbreak, (int _fildes, int _duration));
_PROTOTYPE( int tcdrain, (int _filedes));
_PROTOTYPE( int tcflush, (int _filedef, int _queue_selector));
_PROTOTYPE( int tcflow, (int _filedes, int _action));
_PROTOTYPE( speed_t cfgetispeed, (const struct termios* _termios_p));
_PROTOTYPE( spped_t cfgetospeed, (const struct termios* _termios_p));
_PROTOTYPE( int cfsetispeed, (struct termios* _termios_p, speed_t _speed));
_PROTOTYPE( int cfsetospeed, (struct termios* _termios_p, speed_t _speed));
_PROTOTYPE( int tcgetattr, (int _filedes, struct termios* _termios_p));
_PROTOTYPE( int tcsetattr, (int _filedes, int _opt_actions, const struct termios* _termios_p));

#define cfgetispeed(termios_p)		((termios_p)->c_ispeed)
#define cfgetospeed(termios_p)		((termios_p)->c_ospeed)
#define cfsetispeed(termios_p,speed)	((termios_p)->c_ispeed = (speed),0)
#define cfsetospeed(termios_p,speed)	((termios_p)->c_ospeed = (speed),0)

#ifdef _MINIX
// the following is an extension of the POSIX standard local variables for Minix,
// POSIX compliant programs are inaccessible,so extend when compiling Minix programs

// extension of termious c_iflag bitmap
#define IXANY		0x0800		// output can be continued with any key

// extension of termios c_oflags bitmap. if enable OPOST, active.
#define ONLCR		0x0002		// when output,map NL to CR_NL.
#define XTABS		0x0004		// expand tabs to space
#define ONOEOT		0x0008		// discard EOT (^D) on output

// extension of termios c_lflag bitmap.
#define LFLUSHO		0x0200		// flush output

// extension of c_cc array
#define VREPRINT	11		// cc_c[VREPRINT] (^R)
#define VLNEXT		12		// cc_c[VLNEXT]   (^V)
#define VDISCARD	13		// cc_c[VDISCARD] (^O)

// extension of baudrate setting
#define B57600		0x0100		// 57600  baud
#define B115200		0x0200		// 115200 baud

// the following are the default settings used by the kernel and stty sane.
#define TCTRL_DEF	(CREAD | CS8 | HUPCL)
#define TINPUT_DEF	(BRKINT|ICRNL|IXON|IXANY)
#define TOUTPUT_DEF	(OPOST |ONLCR)
#define TLOCAL_DEF	(ISIG  |IEXTEN|ICANON|ECHO|ECHOE)
#define TSPEED_DEF	B9600

#define TEOF_DEF	'\4'		// ^D
#define TEOL_DEF	_POSIX_VDISABLE
#define TERASE_DEF	'\10'		// ^H
#define TINTR_DEF	'\177'		// ^?
#define TKILL_DEF	'\25'		// ^U
#define TMIN_DEF	1
#define TQUIT_DEF	'\34'		// ^\	;
#define TSTART_DEF	'\21'		// ^Q
#define TSTOP_DEF	'\23'		// ^S
#define TSUSP_DEF	'\32'		// ^Z
#define TTIME_DEF	0
#define TREPRINT_DEF	'\22'		// ^R
#define TLNEXT_DEF	'\26'		// ^V
#define TDISCARD_DEF	'\17'		// ^O

// window size information is stored in the TTY driver but is not used.
// screen-based applications should be available in a window environment.
// use the TIOCGWINSZ and TIOCSWINSZ ioctls to obtain and configure this information.

struct winsize
{
	unsigned short ws_row;		// number of row. character unit
	unsigned short ws_col;		// number of column. character unit
	unsigned short ws_xpixel;	// number of horizontal pixels
	unsigned short ws_ypixel;	// number of vertical pixels
};

#endif // _MINIX

#endif // _TERMIOS_H
