#ifndef _LOWLEVELLOCK_H
#define _LOWLEVELLOCK_H	1

#ifndef __ASSEMBLER__

#define RDI_LP "rdi"
#define RSP_LP "rsp"

#define LOCK_INSTR "lock;"

#define SYS_futex		__NR_futex

#define __lll_lock_asm_start LOCK_INSTR "cmpxchgl %4, %2\n\t"		      \
			      "jz 24f\n\t"


#define lll_lock(futex) \
  (void)								      \
  ({ int ignore1, ignore2, ignore3;           \
       	 __asm __volatile (__lll_lock_asm_start  \
			   "1:\tlea %2, %%" RDI_LP "\n"	     \
			   "2:\tsub $128, %%" RSP_LP "\n"    \
			   ".cfi_adjust_cfa_offset 128\n"    \
			   "3:\tcallq __lll_lock_wait\n"     \
			   "4:\tadd $128, %%" RSP_LP "\n"    \
			   ".cfi_adjust_cfa_offset -128\n"   \
			   "24:"                             \
			   : "=S" (ignore1), "=D" (ignore2), "=m" (futex), "=a" (ignore3)				      \
			   : "1" (1), "m" (futex), "3" (0)     \
			   : "cx", "r11", "cc", "memory");		      \
    })									      \


# define __lll_unlock_asm_start LOCK_INSTR "decl %0\n\t"		      \
				"je 24f\n\t"

#define lll_unlock(futex) \
  (void)								      \
    ({ int ignore;							      \
	 __asm __volatile (__lll_unlock_asm_start			      \
			   "1:\tlea %0, %%" RDI_LP "\n"			      \
			   "2:\tsub $128, %%" RSP_LP "\n"		      \
			   ".cfi_adjust_cfa_offset 128\n"		      \
			   "3:\tcallq __lll_unlock_wake\n"		      \
			   "4:\tadd $128, %%" RSP_LP "\n"		      \
			   ".cfi_adjust_cfa_offset -128\n"		      \
			   "24:"					      \
			   : "=m" (futex), "=&D" (ignore)		      \
			   : "m" (futex) 			      \
               : "ax", "cx", "r11", "cc", "memory");     \
    })

#endif /* __ASSEMBLER__ */

#endif	/* lowlevellock.h */
