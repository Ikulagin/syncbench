#include "sysdep.h"
#include <sys/syscall.h>

#ifdef __ASSEMBLER__
    
#define FUTEX_WAIT  0
#define FUTEX_WAKE  1
#define FUTEX_PRIVATE_FLAG  128
    
#define SYS_futex       __NR_futex    

	.text
    
#define LOAD_FUTEX_WAIT(reg) \
	xorl	$(FUTEX_WAIT | FUTEX_PRIVATE_FLAG), reg

#define LOAD_FUTEX_WAKE(reg) \
	xorl	$(FUTEX_WAKE | FUTEX_PRIVATE_FLAG), reg


    .globl	__lll_lock_wait
	.type	__lll_lock_wait,@function
	.hidden	__lll_lock_wait
	.align	16
__lll_lock_wait:
	cfi_startproc
	pushq	%r10
	cfi_adjust_cfa_offset(8)
	pushq	%rdx
	cfi_adjust_cfa_offset(8)
	cfi_offset(%r10, -16)
	cfi_offset(%rdx, -24)
	xorq	%r10, %r10	/* No timeout.  */
	movl	$2, %edx
	LOAD_FUTEX_WAIT (%esi)

	cmpl	%edx, %eax	/* NB:	 %edx == 2 */
	jne	2f
    nop

1:  movl	$SYS_futex, %eax
	syscall

2:  movl	%edx, %eax
	xchgl	%eax, (%rdi)	/* NB:	 lock is implied */

	testl	%eax, %eax
	jnz	1b

	popq	%rdx
	cfi_adjust_cfa_offset(-8)
	cfi_restore(%rdx)
	popq	%r10
	cfi_adjust_cfa_offset(-8)
	cfi_restore(%r10)
	retq
	cfi_endproc
	.size	__lll_lock_wait,.-__lll_lock_wait


    .globl	__lll_contprof_wait
	.type	__lll_contprof_wait,@function
	.hidden	__lll_contprof_wait
	.align	16
__lll_contprof_wait:
	cfi_startproc
	pushq	%r10
	cfi_adjust_cfa_offset(8)
	pushq	%rdx
	cfi_adjust_cfa_offset(8)
	cfi_offset(%r10, -16)
	cfi_offset(%rdx, -24)
	xorq	%r10, %r10	/* No timeout.  */
	movl	$2, %edx
	LOAD_FUTEX_WAIT (%esi)

	cmpl	%edx, %eax	/* NB:	 %edx == 2 */
	jne	2f
    nop

1:  lock subl $0x1,0x4(%rdi)
    pushq %rcx
    movl	$SYS_futex, %eax
	syscall

    popq %rcx
    movl $0x1, %eax 
    lock xadd %eax,0x4(%rdi)
    cltq
    addl   $0x1,(%rcx,%rax,4)
        
2:  movl	%edx, %eax
	xchgl	%eax, (%rdi)	/* NB:	 lock is implied */

	testl	%eax, %eax
	jnz	1b

	popq	%rdx
	cfi_adjust_cfa_offset(-8)
	cfi_restore(%rdx)
	popq	%r10
	cfi_adjust_cfa_offset(-8)
	cfi_restore(%r10)
	retq
	cfi_endproc
	.size	__lll_contprof_wait,.-__lll_contprof_wait

        
	.globl	__lll_unlock_wake
	.type	__lll_unlock_wake,@function
	.hidden	__lll_unlock_wake
	.align	16
__lll_unlock_wake:
	cfi_startproc
	pushq	%rsi
	cfi_adjust_cfa_offset(8)
	pushq	%rdx
	cfi_adjust_cfa_offset(8)
	cfi_offset(%rsi, -16)
	cfi_offset(%rdx, -24)

	movl	$0, (%rdi)
	LOAD_FUTEX_WAKE (%esi)
	movl	$1, %edx	/* Wake one thread.  */
	movl	$SYS_futex, %eax
	syscall

	popq	%rdx
	cfi_adjust_cfa_offset(-8)
	cfi_restore(%rdx)
	popq	%rsi
	cfi_adjust_cfa_offset(-8)
	cfi_restore(%rsi)
	retq
	cfi_endproc
	.size	__lll_unlock_wake,.-__lll_unlock_wake

#endif /* _ASSEMBLER__ */
