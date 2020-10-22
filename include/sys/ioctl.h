// <sys/ioctl.h> header defines most macros used to process the device control.
// and, it includes prototype of IOCTL system call.
//
// declare device control operations
//

#ifndef _IOCTL_H
#define _IOCTL_H

#if _EM_WSIZE >= 4
// ioctl uses lower words as commands and higher words as parameter sizes.
// in addition, the upper 3 bits of the upper word of the parameter state(in/out/void)
// are used.

#define _IOCPARM_MASK		0x1FFF
#define _IOC_VOID		0x20000000
#define _IOCTYPE_MASK		0xFFFF
#define _IOC_IN			0x40000000
#define _IOC_OUT		0x80000000
#define _IOC_INOUT		(_IOC_IN | _IOC_OUT)

#define _IO(x,y)		((x << 8) | y | _IOC_VOID)
#define _IOR(x,y,t)		((x << 8) | y | ((sizeof(t) & _IOCPARM_MASK) << 16) | _IOC_OUT)
#define _IOW(x,y,t)		((x << 8) | y | ((sizeof(t) & _IOCPARM_MASK) << 16) | _IOC_IN)
#define _IORW(x,y,t)		((x << 8) | y | ((sizeof(t) & _IOCPARM_MASK) << 16) | _IOC_INOUT)

#else
// advanced on 16 bit machines there is no coding

#define _IO(x,y)		((x << 8) | y)
#define _IOR(x,y,t)		_IO(x,y)
#define _IOW(x,y,t)		_IO(x,y)
#define _IORW(x,y,t)		_IO(x,y)

// device ioctl
#define TCGETS			_IOR('T', 8,struct termios)	// tcgetattr
#define TCSETS			_IOW('T', 9,struct termios)	// tcsetattr , TCSANOW
#define TCSETSW			_IOW('T',10,struct termios)	// tcsetattr , TCSADRAIN
#define TCSETSF			_IOW('T',11,struct termios)	// tcsetattr , TCSAFLUSH
#define TCSBRK			_IOW('T',12,int)		// tcsendbreak
#define TCDRAIN			_IO('T',13)			// tcdrain
#define TCFLOW			_IOW('T',14,int)		// tcflow
#define TCFLSH			_IOW('T',15,int)		// tcflush
#define TIOCGWINSZ		_IOR('T',16,struct winsize)
#define TIOCSWINSZ		_IOW('T',17,struct winsize)
#define TIOCGPGRP		_IOW('T',18,int)
#define TIOCSPGRP		_IOW('T',19,int)
#define TIOCSFON		_IOW('T',20,u8_t [8192])

#define TIOCGETP		_IOR('t', 1,struct sgttyb)
#define TIOCSETP		_IOW('t', 2,struct sgttyb)
#define TIOCGETC		_IOR('t', 3,struct tchars)
#define TIOCSETC		_IOW('t'. 4,struct tchars)

// network ioctl
#define NWIOSETHOPT		_IOW('n',16,struct nwio_ethopt)
#define NWIOGETHOPT		_IOR('n',17,struct nwio_ethopt)
#define NWIOGETHSTAT		_IOR('n',18,struct nwio_ethstat)

#define NWIOSIPCONF		_IOW('n',32,struct nwio_ipconf)
#define NWIOGIPCONF		_IOR('n',33,struct nwio_ipconf)
#define NWIOSIPORT		_IOW('n',34,struct nwio_ipopt)
#define NWIOGIPORT		_IOR('n',35,struct nwio_ipopt)

#define NWIOIPGROUTE		_IORW('n',40,struct nwio_route)
#define NWIOIPSROUTE		_IOW('n',41,struct nwio_route)
#define NWIOIPDROUTE		_IOW('n',42,struct nwio_route)

#define NWIOSTCPCONF		_IOW('n',48,struct nwio_tcpconf)
#define NWIOGTCPCONF		_IOR('n',49,struct nwio_tcpconf)
#define NWIOTCPCONN		_IOW('n',50,struct nwio_tcpcl)
#define NWIOTCPLISTEN		_IOW('n',51,struct nwio_tcpcl)
#define NWIOTCPATTACH		_IOW('n',52,struct nwio_tcpatt)
#define NWIOTCPSHUTDOWN		_IO('n',53)
#define NWIOSTCPOPT		_IOW('n',54,struct nwio_tcpopt)
#define NWIOGTCPOPT		_IOR('n',55,struct nwio_tcpopt)

#define NWIOSUDPOPT		_IOW('n',64,struct nwio_udpopt)
#define NWIOGUDPOPT		_IOR('n',65,struct nwio_udpopt)

// disk ioctl
#define DIOCEJECT		_IO('d',5)
#define DIOCSETP		_IOW('d',6,struct partition)
#define DIOCGETP		_IOR('d',7,struct partition)

// keyboard ioctl
#define KIOCSMAP		_IOW('k',3,keymap_t)

// memory ioctl
#define MIOCRAMSIZE		_IOW('m',3,u32_t) // size of RAM disk
#define MIOCSPSINFO		_IOW('m',4,void*)
#define MIOCGPSINFO		_IOR('m',5,struct psinfo)

// magnetic tape ioctl
#define MTIOCTOP		_IOW('M',1,struct mtop)
#define MTIOCGET		_IOR('M',2,struct mtget)

// SCSI commands
#define SCIOCCMD		_IOW('S',1,struct scsicmd)

// CD_ROM ioctl
#define CDIOPLAYTI		_IOR('c',1,struct cd_play_track)
#define CDIOPLAYMSS		_IOR('c',2,struct cd_play_mss)
#define CDIOREADTOCHDR		_IOW('c',3,struct cd_toc_entry)
#define CDIOREADTOC		_IOW('c',4,struct cd_toc_entry)
#define CDIOREADSUBCH		_IOW('c',5,struct cd_toc_entry)
#define CDIOSTOP		_IO('c',10)
#define CDIOPAUSE		_IO('c',11)
#define CDIORESUME		_IO('c',12)
#define CDIOEJECT		DIOCEJECT

// sound card DSP ioctl
#define DSPIORATE		_IOR('s',1,unsigned int)
#define DSPIOSTEREO		_IOR('s',2,unsigned int)
#define DSPIOSIZE		_IOR('s',3,unsigned int)
#define DSPIOBITS		_IOR('s',4,unsigned int)
#define DSPIOSIGN		_IOR('s',5,unsigned int)
#define DSPIOMAX		_IOW('w',6,unsigned int)
#define DSPIORESET		_IO('s',7)

// soundcard mixer ioctl
#define MIXIOGETVOLUME		_IORW('s',10,struct volume_level)
#define MIXIOGETINPUTLEFT	_IORW('s',11,struct inout_ctrl)
#define MIXIOGETINPUTRIGHT	_IORW('s',12,struct inout_ctrl)
#define MIXIOGETOUTPUT		_IORW('s',13,struct inout_ctrl)
#define MIXIOSETVOLUME		_IORW('s',14,struct volume_level)
#define MIXIOSETINPUTLEFT	_IORW('s',15,struct inout_ctrl)
#define MIXIOSETINPUTRIGHT	_IORW('s',16,struct inout_ctrl)
#define MIXIOSETOUTPUT		_IORW('s',17,struct inout_ctrl)

#ifndef _ANSI
#include <ansi.h>
#endif // _ANSI

_PROTOTYPE(int ioctl,(int _fd,int _request,void* _data));

#endif // _IOCTL_H


