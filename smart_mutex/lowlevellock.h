#ifndef _LOWLEVELLOCK_H
#define _LOWLEVELLOCK_H	1

#include "sysdep.h"

#ifndef __ASSEMBLER__

#define SYS_futex		__NR_futex

#define LOCK_INSTR "lock;"

/* Initializer for lock.  */
#define LLL_LOCK_INITIALIZER        (0)
#define LLL_LOCK_INITIALIZER_LOCKED (1)
#define LLL_LOCK_INITIALIZER_WAITERS    (2)

/* NB: in the lll_trylock macro we simply return the value in %eax
   after the cmpxchg instruction.  In case the operation succeded this
   value is zero.  In case the operation failed, the cmpxchg instruction
   has loaded the current value of the memory work which is guaranteed
   to be nonzero.  */
# define __lll_trylock_asm LOCK_INSTR "cmpxchgl %2, %1"

#define lll_trylock(futex) \
    ({ int ret;                                     \
        __asm __volatile (__lll_trylock_asm                      \
                          : "=a" (ret), "=m" (futex)                 \
                          : "r" (LLL_LOCK_INITIALIZER_LOCKED), "m" (futex),      \
                          "0" (LLL_LOCK_INITIALIZER)               \
                          : "memory");                       \
        ret; })

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
			   : "=S" (ignore1), "=D" (ignore2), "=m" (futex),\
                 "=a" (ignore3)                     \
               : "1" (1), "m" (futex), "3" (0), "0" (0)  \
			   : "cx", "r11", "cc", "memory");		      \
    })									      \


#define __lll_unlock_asm_start LOCK_INSTR "decl %0\n\t"		      \
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
               : "m" (futex), "S" (0)              \
               : "ax", "cx", "r11", "cc", "memory");     \
    })

#endif /* __ASSEMBLER__ */

#endif	/* lowlevellock.h */
