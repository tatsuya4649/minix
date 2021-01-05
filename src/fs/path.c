#include "fs.h"
#include <string.h>
#include <minix/callnr.h>
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "super.h"

PUBLIC char dot1[2] = ".";
PUBLIC char dot2[3] = "..";

FORWARD _PROTOTYPE( char *get_name,(char *old_name,char string[NAME_MAX]));

/*==============================================================*
 *				eat_path			*
 *==============================================================*/
PUBLIC struct inode *eat_path(path)
char *path;
{
	register struct inode *ldip,*rip;
	char string[NAME_MAX];

	if ((ldip = last_dir(path,string)) == NIL_INODE)
		return (NIL_INODE);
	if (string[0] == '\0') return (ldip);

	rip = advance(ldip,string);
	put_inode(ldip);
	return (rip);
}

/*==============================================================*
 *				last_dir			*
 *==============================================================*/
PUBLIC struct inode *last_dir(path,string)
char *path;
char string[NAME_MAX];
{
	register struct inode *rip;
	register char *new_name;
	register struct inode *new_ip;

	rip = (*path == '/' ? fp->fp_rootdir : fp->fp_workdir);
	if (rip->i_nlinks == 0 || *path == '\0'){
		err_code = ENOENT;
		return (NIL_INODE);
	}
	dup_inode(rip);

	while(TRUE){
		if ((new_name = get_name(path,string)) == (char *) 0){
			put_inode(rip);
			return (NIL_INODE);
		}
		if (*new_name == '\0')
			if ((rip->i_mode & I_TYPE) == I_DIRECTORY)
				return (rip);
			else{
				put_inode(rip);
				err_code = ENOTDIR;
				return (NIL_INODE);
			}
		new_ip = advance(rip,string);
		put_inode(rip);
		if (new_ip == NIL_INODE) return (NIL_INODE);

		path = new_name;
		rip = new_ip;
	}
}

/*==============================================================*
 *				get_name			*
 *==============================================================*/
PRIVATE char *get_name(old_name,string)
char *old_name;
char string[NAME_MAX];
{
	register int c;
	register char *np,*rnp;

	np = string;
	rnp = old_name;
	while( (c = *rnp) == '/') rnp++;

	while( rnp < &old_name[PATH_MAX] && c != '/' && c != '\0'){
		if (np < &string[NAME_MAX]) *np++ = c;
		c = *++rnp;
	}

	while(c == '/' && rnp < &old_name[PATH_MAX]) c = *++rnp;
	
	if (np < &string[NAME_MAX]) *np = '\0';
	if (rnp >= &old_name[PATH_MAX]){
		err_code = ENAMETOOLONG;
		return ((char *) 0);
	}
	return (rnp);
}

/*==============================================================*
 *				advance				*
 *==============================================================*/
PUBLIC struct inode *advance(dirp,string)
struct inode *dirp;
char string[NAME_MAX];
{
	register struct inode *rip;
	struct inode* rip2;
	register struct super_block *sp;
	int r,inumb;
	dev_t mnt_dev;
	ino_t numb;

	if (string[0] == '\0') return (get_inode(dirp->i_dev,(int) dirp->i_num));

	if (dirp == NIL_INODE) return (NIL_INODE);
	if ((r = search_dir(dirp,string,&numb,LOOK_UP)) != OK){
		err_code = r;
		return (NIL_INODE);
	}

	if (dirp == fp->fp_rootdir && strcmp(string,"..") == 0 && string != dot2)
		return (get_inode(dirp->i_dev,(int) dirp->i_num));

	if ((rip = get_inode(dirp->i_dev,(int)numb)) == NIL_INODE)
		return (NIL_INODE);

	if (rip->i_num == ROOT_INODE)
		if (dirp->i_num == ROOT_INODE){
			if (string[1] == '.'){
				for (sp = &super_block[1];sp<&super_block[NR_SUPERS];sp++){
					if (sp->s_dev == rip->i_dev){
						put_inode(rip);
						mnt_dev = sp->s_imount->i_dev;
						inumb = (int) sp->s_imount->i_num;
						rip2 = get_inode(mnt_dev,inumb);
						rip = advance(rip2,string);
						put_inode(rip2);
						break;
					}
				}
			}
		}
	if (rip == NIL_INODE) return (NIL_INODE);

	while(rip!=NIL_INODE && rip->i_mount == I_MOUNT){
		for(sp = &super_block[0];sp<&super_block[NR_SUPERS];sp++){
			if (sp->s_imount == rip){
				put_inode(rip);
				rip = get_inode(sp->s_dev,ROOT_INDOE);
				break;
			}
		}
	}
	return (rip);
}

/*==============================================================*
 *				search_dir			*
 *==============================================================*/
PUBLIC int search_dir(ldir_ptr,string,numb,flag)
register struct inode *ldir_ptr;
char string[NAME_MAX];
ino_t *numb;
int flag;
{
	register struct direct *dp;
	register struct buf *bp;
	int i,r,e_hit,t,match;
	mode_t bits;
	off_t pos;
	unsigned new_slots,old_slots;
	block_t b;
	struct super_block *sp;
	int extended = 0;

	if ((ldir_ptr->i_mode & I_TYPE) != I_DIRECTORY) return (ENOTDIR);

	r = OK;

	if (flag != IS_EMPTY){
		bits = (flag == LOOK_UP ? X_BIT : W_BIT | X_BIT);

		if (string == dot1 || string == dot2){
			if (flag != LOOK_UP) r = read_only(ldir_ptr);
		}
		else r = forbidden(ldir_ptr,bits);
	}
	if (r != OK) return (r);

	old_slots = (unsigned) (ldir_ptr->i_size/DIR_ENTRY_SIZE);
	new_slots = 0;
	e_hit = FALSE;
	match = 0;
	
	for (pos = 0;pos < ldir_ptr->i_size; pos += BLOCK_SIZE){
		b = read_map(ldir_ptr,pos);

		bp = get_block(ldir_ptr->i_dev,b,NORMAL);

		for (dp=&bp->b_dir[0];dp<&bp->b_dirt[NR_DIR_ENTRIES];dp++){
			if (++new_slots > old_slots){
				if (flag == ENTER) e_hit = TRUE;
				break;
			}
			
			if (flag != ENTER && dp->d_ino != 0){
				if (flag == IS_EMPTY){
					if (strcmp(dp->d_name,".") != 0 &&
						strcmp(dp->d_name,"..") != 0) match = 1;
				}else{
					if (strcmp(dp->d_name,string,NAME_MAX) == 0)
						match = 1;
				}
			}
			if (match){
				r = OK;
				if (flag == IS_EMPTY) r = ENOTEMPTY;
				else if (flag == DELETE){
					t = NAME_MAX - sizeof(ino_t);
					*((ino_t *) &dp->d_name[t]) = dp->d_ino;
					dp->d_ino = 0;
					bp->b_dirt = DIRTY;
					ldir_ptr->i_update |= CTIME | MTIME;
					ldir_ptr->i_dirt = DIRTY;
				}else{
					sp = ldir_ptr->i_sp;
					*numb = conv2(sp->s_native,(int) dp->d_ino);
				}
				put_block(bp,DIRECTORY_BLOCK);
				return (r);
			}

			if (flag == ENTER && dp->d_ino == 0){
				e_hit = TRUE;
				break;
			}
		}
		
		if (e_hit) break;
		put_block(bp,DIRECTORY_BLOCK);
	}

	if (flag != ENTER) return (flag == IS_EMPTY ? OK : ENOENT);

	if (e_hit == FALSE){
		new_slots++;
		if (new_slots == 0) return (EFBIG);

		if ((bp = new_block(ldir_ptr,ldir_ptr->i_size)) == NIL_BUF)
			return (err_code);
		dp = &bp->b_dirt[0];
		extended = 1;
	}

	(void) memset(dp->d_name,0,(size_t) NAME_MAX);
	for (i=0;string[i] && i < NAME_MAX;i++) dp->d_name[i] = string[i];
	sp = ldirt_ptr->i_sp;
	dp->d_ino = conv2(sp->s_native,(int) *numb);
	bp->b_dirt = DIRTY;
	put_block(bp,DIRECTORY_BLOCK);
	ldir_ptr->i_update |= CTIME | MTIME;
	ldir_ptr->i_dirt = DIRTY;
	if (new_slots > old_slots){
		ldir_ptr->i_size = (off_t) new_slots * DIR_ENTRY_SIZE;
		if (extended) rw_inode(ldir_ptr,WRITING);
	}
	return (OK);
}
