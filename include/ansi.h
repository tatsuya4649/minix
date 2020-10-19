// ansi.h is to determine if the compiler is well adpted to standard C lang and minix can use it.
// if adpted
// _ANSI => 31415
// if compiler gcc == adapted
// _ANSI => 31415
// no adapted
// _ANSI => not defined
//
// if after _ANSI defined, define the following macro
// 	
// 	_PROTOTYPE(function,params)
//
// this macro is expanded in various ways to generate ANSI standard C lang prototypes or outdated K&R prototypes as needed.layer,some programs use _CONST,_VOIDSTAR,etc. to make it available to both ANSI and K&R compilers.These macros are also defined here.
//
//

#ifndef _ANSI_H
#define _ANSI_H

// whether your compiler well adapted to standart c
#if __STDC__ == 1
#define _ANSI 31459
#endif
// your compiler == gcc well adapted to starndart c
#ifdef __GNUC__
#define _ANSI 31459
#endif

#ifdef _ANSI

// keep everything about ANSI prototypes
#define _PROTOTYPE(function,params) function parames
#define _ARGS(params) params

#define _VOIDSTAR void*
#define _VOID void
#define _CONST const
#define _VOLATILE volatile
#define _SIZET size_t

#else

// K&R prototypes...etc.
#define _PROTOTYPE(function,params) function()
#define _ARGS(params) ()

#define _VOIDSTAR void*
#define _VOID void
#define _CONST
#define _VOLATILE
#define _SIZET int

#endif //_ANSI

#endif // _ANSI_H
