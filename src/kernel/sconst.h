! rel and other constants used in the assembler code.
W		= 	_WORD_SIZE		! machine size

! offset of struct process. must match proc.h
P_STACKBASE 	= 	0
#if _WORD_SIZE == 2
ESREG		=	P_STACKBASE
#else
GSREG		= 	P_STACKBASE
FSREG		= 	GSREG + 2		! 386 introduces FS and GS segment
ESREG		= 	FSREG + 2
#endif
DSREG		= 	ESREG + 2
DIREG		=	DSREG + 2
SIREG		= 	DIREG + W
BPREG		=	SIREG + W
STREG		=	BPREG + W		! spare for another SP
BXREG		=	STREG + W
DXREG		=	BXREG + W
CXREG		= 	DXREG + W
AXREG		=	CXREG + W
RETADR		=	AXREG + W		! return address of save() call
PCREG		= 	RETADR + W
CSREG		=	PCREG + W
PSWREG		=	CSREG + W
SPREG		=	PSWREG + W
SSREG		=	SPREG + W
P_STACKTOP	= 	SSREG + W
P_LDT_SEL	=	P_STACKTOP
P_LDT		=	P_LDT_SEL + W

#if _WORD_SIZE == 2
Msize 		=	12			! size of message for 16bit words
#else
Msize		=	9			! size of message for 32bit words
#endif
