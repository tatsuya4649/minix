// <string.h> header have prototype of about "string" processing function
//

#ifndef _STRING_H
#define _STRING_H

#define NULL ((void*)0)

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t; // type receive from sizeof
#endif // _SIZE_T

#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE( void* memchr,( const void* _s,int _c,size_t _n));
_PROTOTYPE( int memcmp,( const void* _s1,const void* _s2,size_t _n));
_PROTOTYPE( void* memcpy, (void* _s1,const void* _s2,size_t _n));
_PROTOTYPE( void* memmove, (void* _s1,const void* _s2,size_t _n));
_PROTOTYPE( void* memset, (void* _s,int _c,size_t _n));
_PROTOTYPE( char* strcat, (char* _s1,char* _s2));
_PROTOTYPE( char* strchr, (const char* _s,int _c));
_PROTOTYPE( int strncmp, (const char* _s1,const char* _s2,size_t _n));
_PROTOTYPE( int strcmp, (const char* _s1,const char* _s2));
_PROTOTYPE( int strcoll, (const char* _s1,const char* _s2));
_PROTOTYPE( char* strcopy,(char* _s1,const char* _s2));
_PROTOTYPE( size_t strcspn, (const char* _s1,const char* _s2));
_PROTOTYOE( char* strerror,(int _errnum));
_PROTOTYPE( char* strlen,(const char* _s));
_PROTOTYPE( char* strncat, (char* _s1,const char* _s2,size_t _n));
_PROTOTYPE( char* strncpy, (char* _s1,const char* _s2,size_t _n));
_PROTOTYPE( char* strpbrk, (const char* _s1,const char* _s2));
_PROTOTYPE( char* strrchr, (const char* _s,int _c));
_PROTOTYPE( size_t strspn,(const char* _s1,const char* _s2));
_PROTOTYPE( char* strstr,(const char* _s1,const char* _s2));
_PROTOTYPE( char* strtok,(char* _s1,const char* _s2));
_PROTOTYPE( size_t strxfrm, (char* _s1,const char* _s2,size_t n));

#ifdef _MINIX
// prototype for backward compatibility
_PROTOTYPE( char* index,(const char* _s,int _charwanted));
_PROTOTYPE( char* rindex,(const char* _s,int _charwanted));
_PROTOTYPE( void bcopy,(const void* _src,void* _dst,size_t _lenght));
_PROTOTYPE( int bcmp,(const void* _s1,const void* _s2,size_t _lenght));
_PROTOTYPE( void bzero,(void* _dst,size_t _length));
_PROTOTYPE( void* memccpy,(char* _dst,const char* _src,int _ucharstop,size_t _size));

//BSD function
_PROTOTYPE( int strcasecmp,(const char* _s1,const char* _s2));
#endif // _MINIX

#endif // _STRING_H
