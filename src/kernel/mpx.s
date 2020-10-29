#
! Minix choose from 8086 and 386 versions of startup code

#include <minix/config.h>
#if _WORD_SIZE == 2
#include "mpx88.s"
#else
#include "mpx386.s"
#endif ! _WORD_SIZE == 2
