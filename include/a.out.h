// <a.out.h> header define the file format
// for storing excutable programs on disk
//
//

#ifndef _AOUT_H
#define _AOUT_H

struct exec{					// a.out header
	unsigned char 	a_magic[2];		// magic number
	unsigned char 	a_flags;		// flags, see below
	unsigned char 	a_cpu;			// cpu id
	unsigned char 	a_hdrlen;		// header length
	unsigned char 	a_unused;		// book for the future
	unsigned short 	a_version;		// version stamp (no used now)
	long 		a_text;			// size of text segment (byte unit)
	long 		a_data;			// size of data segment (byte unit)
	long 		a_bss;			// size of bss segment (byte unit)
	long 		a_entry;		// entry point
	long 		a_total;		// total number of memory allocated
	long 		a_syms;			// symbol table size
	// if short format,end here
	long		a_trsize;		// text rearrangement size
	long 		a_drsize;		// data rearrangement size
	long		a_tbase;		// text rearrangement base
	long 		a_dbase;		// data rearrangement base
};

#define A_MAGIC0	(unsigned char) 0x01
#define A_MAGIC1	(unsigned char) 0x03
#define BADMAG(X)	((X).a_magic[0] != A_MAGIC0 || (X).a_magic[1] != A_MAGIC1)

// CPU ID of TARGET machine (ID coded in the lower 2bits)
#define A_NONE 		0x00 			// unknown
#define A_I8086		0x04			// Intel i8086/8088
#define A_M68K		0x0B			// Motorola m68000
#define A_NS16K		0x0C			// National Semiconductor 16032
#define A_I80386	0x10			// Intel i80386
#define A_SPARC		0x17			// Sun SPARC

#define A_BLR(cputype)	((cputype&0x01)!=0)	// True if the bytes are from left to right
#define A_WLR(cputype) 	((cputype&0x02)!=0)	// True if the words are from left to right

// flags
#define A_UZP		0x01			// non-mapping zero page
#define A_PAL		0x02			// aligned executable page
#define A_NSYM		0x04			// new style symbol table
#define A_EXEC		0x10			// executable
#define A_SEP		0x20			// split I/D
#define A_PURE		0x40			// pure text (no use)
#define A_TOVLY		0x80			// text overlay (no use)

// various offsets
#define A_MINHDR	32
#define A_TEXTPOS(X)	((long)(X).a_hdrlen)
#define A_DATAPOS(X) 	(A_TEXTPOS(X) + (X).a_text)
#define A_HASRELS(X)	((X).a_hdrlen > (unsigned char) A_MINHDR)
#define A_HASEXT(X)	((X).a_hdrlen > (unsigned char) (A_MINHDR + 8))
#define A_HASLNS(X)	((X).a_hdrlen > (unsigned char) (A_MINHDR + 16))
#define A_HASTOFF(X)	((X).a_hdrlen > (unsigned char) (A_MINHDR + 24))
#define A_TRELPOS(X)	(A_DATAPOS(X) + (X).a_data)
#define A_DRELPOS(X)	(A_TRELPOS(X) + (X).a_trsize)
#define A_SYMPOS(X)	(A_TRELPOS(X) + (A_HASRELS(X) ? ((X).a_trsize + (X).a_drsize) : 0))

struct reloc{
	long r_vaddr;			// virtual address for reference
	unsigned short r_symndx;	// internal segment number or external symbol number
	unsigned short r_type;		// relocation type
};

// r_type value
#define R_ABBS		0
#define R_RELLBYTE	2
#define R_PCRBYTE	3
#define R_RELWORD	4
#define R_PCRWORD	5
#define R_RELLONG	6
#define R_PCRLONG	7
#define R_REL3BYTE	8
#define R_KBRANCHE	9

// r_symndx of internal segment
#define S_ABS		((unsigned short)-1)
#define S_TEXT		((unsigned short)-2)
#define S_DATA		((unsigned short)-3)
#define S_BSS		((unsigned short)-4)

struct nlist{			// symbol table entry
	char n_name[8];		// symbol name
	long n_value;		// value
	unsigned char n_sclass;	// memory class
	unsigned char n_numaux;	// number of auxiliary entories (no use)
	unsigned short n_type;	// basic and derived types of languafe (no use)
};

// lower bits of storage class (section)
#define N_SECT		07	// section mask
#define N_UNDF		01	// undefined
#define N_ABS		02	// absolute
#define N_TEXT		03	// text
#define N_DATA		04	// data
#define N_BSS		05	// bss
#define N_COMM		06	// (common)

// upper bits of storage class
#define N_CLASS		0370	// storage class mask
#define C_NULL
#define C_EXT		0020	// external symbol
#define C_STAT		0030	// static

// function prototype
#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

_PROTOTYPE(int nlist, (char* _file,struct nlist* _nl));

#endif // _AOUT_H