// <sys/sigcontext.h> header defines a structure for retaing and restoring
// normal system processing before and after executing a signal processing routine,
// and is used by both the kernel and memory manager.
//
//

#ifdef _SIGCONTEXT_H
#define _SIGCONTEXT_H

// sigcontext structure is used by the sigreturn(2) system call.
// sigreturn() is rarely called by the user program and is used
// internally by the signal acquisition mechanism.
//

#ifndef _ANSI_H
#include <ansi.h>
#endif // _ANSI_H

#ifndef _CONFIG_H
#include <minix/config.h>
#endif // _CONFIG_H

#if !defined(CHIP)
#include "error,configuration is not known"
#endif // !defined(CHIP)

// 

#endif // _SIGCONTEXT_H
