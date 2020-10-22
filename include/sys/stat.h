// <sys/stat.h> header defines struct user in stat() and fstat function.
// this struct info is constructed from i node of file.
// stat() and fstat function is the only way to inspect inode.
//
//

#ifndef _STAT_H
#define _STAT_H

struct stat{
	dev_t 		st_dev;		// major/minor device number
	ino_t 		st_ino;		// inode number
	mode_t 		st_mode;	// file mode, ptotect bits etc
	short int 	st_nlink;	// number of links : originally,it should be nlink_t
	uid_t		st_uid;		// uid of file owner
	short int	st_gid;		// gid; originally,it should be gid_t
	dev_t		st_rdev;
	off_t		st_size;	// file size
	time_t		st_atime;	// last access time
	time_t		st_mtime;	// last update time
	time_t		st_ctime;	// last information update time
};

// tranditional mask definition for st_mode
// the cast attached to part of the definition is to avoid unexpected
// symbolic extentions such as S_IFREG != (mode_t) S_IFREG when int is 32 bits.
#define S_IFMT		((mode_t) 0170000)	// file type
#define S_IFREG		((mode_t) 0100000)	// normal
#define S_IFBLK		0060000			// block special
#define S_IFDIR		0040000			// directory
#define S_IFCHR		0020000			// character special
#define S_IFIFO		0010000			// FIFO
#define S_ISUID		0004000			// set user id at runtime
#define S_ISGIU		0002000			// set group id at runtime
						// next book for future use
#define S_ISVTX		01000			// save swapped text even after use

// st_mode POSIX mask
#define S_IRWXU		00700			// owner: rwx------
#define S_IRUSR		00400			// owner: r--------
#define S_IWUSR		00200			// owner: -w-------
#define S_IXUSR		00100			// owner: --x------

#define S_IRWXG		00070			// group: ---rwx---
#define S_IRGRP		00040			// group: ---r-----
#define S_IWGRP		00020			// group: ----w----
#define S_IXGRP		00010			// group: -----x---

#define S_IRWXO		00007			// other: ------rwx
#define S_IROTH		00004			// other: ------r--
#define S_IWOTH		00002			// other: -------w-
#define S_IXOTH		00001			// other: --------x

// the following macros are tested in st_mode
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)	// it is a normal file
#define S_ISDIE(m)	(((m) & S_IFMT) == S_IFDIR)	// it is a directory
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)	// it is a character specifications
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)	// it is a block specifications
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	// it is a pipe/FIFO

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE( int chmod,(const char *_path,Mode_t _mode));
_PROTOTYPE( int fstat,(int _fildes,struct stat *_buf));
_PROTOTYPE( int mkdir,(const char *_path,Mode_t _mode));
_PROTOTYPE( int mkfifo,(const char *_path,Mode_t _mode));
_PROTOTYPE( int stat,(const char *_path,struct stat *_buf));
_PROTOTYPE( mode_t umask,(Mode_t _cmask));

#endif // _STAT_H
