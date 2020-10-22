// <sys.dir> header define the structure of minix directory entries.
// this is a one-time direct reference, but this reference is from
// another header that is widely used in the file system.
//
//

#ifndef _DIR_H
#define _DIR_H

#define DIRBLKSIZ	512		// size of directory block

#ifndef DIRSIZ
#define DIRSIZ		14
#endif

struct direct{
	ino_t 		d_ino;
	char		d_name[DIRSIZ];
};

#endif // _DIR_H
