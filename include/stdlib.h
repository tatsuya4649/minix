// <stdlib.h> header define feneric macros,types,functions.
//
//

#ifndef _STDLIB_H
#define _STDLIB_H

// macro include NULL,EXIT_FAILURE,EXIT_SUCCESS,RAND_MAX,and MB_CUR_MAX
#define NULL 		((void*)0)
#define EXIT_FAILURE 	1		// standard error return value used in exit()
#define EXIT_SUCCESS	0		// standard success return value used in exit()
#define RAND_MAX	32767		// maximum value of rand()
#define MB_CUR_MAX	1		// maximum value of minix multibyte characters

typedef struct { int quot,rem; } div_t;
typedef struct { long quot,rem; } ldiv_t;

// about types of size_t,wchar_t,div_t,ldiv_t
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;		// return value of sizeof
#endif // _SIZE_T

#ifndef _WCHAR_T
#define _WCHAR_T
typedef char wchar_t;			// expanded character set type
#endif // _WCHAR_T

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE( void abort, (void));
_PROTOTYPE( int abs, (int _j));
_PROTOTYPE( int atexit, (void (*_func)(void)));
_PROTOTYPE( double atof, (const char* _nptr));
_PROTOTYPE( int atoi, (const char* _nptr));
_PROTOTYPE( long atol, (const char* _nptr));
_PROTOTYPE( void* calloc, (size_t _nmemb, size_t _size));
_PROTOTYPE( div_t div, (int _numer,int _denom));
_PROTOTYPE( void exit, (int _status));
_PROTOTYPE( void free, (void* _ptr));
_PROTOTYPE( char* getenv, (const char* _name));
_PROTOTYPE( long labs, (long _j));
_PROTOTYPE( ldiv_t ldiv, (long _numer, long _denom));
_PROTOTYPE( void* malloc, (size_t _size));
_PROTOTYPE( int mblen, (const char* _s, size_t _n));
_PROTOTYPE( size_t mbstowcs, (wchar_t* _pwc, const char* _s, size_t _n));
_PROTOTYPE( int mbtowc, (wchar_t* _pwc, const char* _s, size_t _n));
_PROTOTYPE( int rand, (void));
_PROTOTYPE( void* realloc, (void* _ptr, size_t _size));
_PROTOTYPE( void srand, (unsigned int _seed));
_PROTOTYPE( double strtod, (const char* _nptr, char** _endptr));
_PROTOTYPE( long strtol, (const char* _nptr, char** _endptr, int _base));
_PROTOTYPE( int system, (const char* _string));
_PROTOTYPE( size_t wcstombs, (char* _s, const wchar_t* _pwcs, size_t _n));
_PROTOTYPE( int wctomb, (char* _s, wchar_t _wchar));
_PROTOTYPE( void* bsearch, (const void* _key, const void* _base, size_t _nmemb, size_t _size, int (*compar)(const void*, const void*)));
_PROTOTYPE( void qsort, (void* _base, size_t _nmemb, size_t _size,int (*compar)(const void*, const void*)));
_PROTOTYPE( unsigned long int strtoul, (const char* _nptr, char** _endptr, int _base));

#ifdef _MINIX
_PROTOTYPE( int putenv, (const char* _name));
_PROTOTYPE( int getopt, (int _argc, char** _argv, char* _opts));
extern char* optarg;
extern int optind,opterr,optopt;
#endif // _MINIX

#endif // _STDLIB_H
