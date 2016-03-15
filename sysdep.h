#ifndef _X86_64_SYSDEP_H
#define _X86_64_SYSDEP_H 1

#ifdef	__ASSEMBLER__

# define cfi_startproc			.cfi_startproc
# define cfi_endproc			.cfi_endproc
# define cfi_def_cfa(reg, off)		.cfi_def_cfa reg, off
# define cfi_def_cfa_register(reg)	.cfi_def_cfa_register reg
# define cfi_def_cfa_offset(off)	.cfi_def_cfa_offset off
# define cfi_adjust_cfa_offset(off)	.cfi_adjust_cfa_offset off
# define cfi_offset(reg, off)		.cfi_offset reg, off
# define cfi_rel_offset(reg, off)	.cfi_rel_offset reg, off
# define cfi_register(r1, r2)		.cfi_register r1, r2
# define cfi_return_column(reg)	.cfi_return_column reg
# define cfi_restore(reg)		.cfi_restore reg
# define cfi_same_value(reg)		.cfi_same_value reg
# define cfi_undefined(reg)		.cfi_undefined reg
# define cfi_remember_state		.cfi_remember_state
# define cfi_restore_state		.cfi_restore_state
# define cfi_window_save		.cfi_window_save
# define cfi_personality(enc, exp)	.cfi_personality enc, exp
# define cfi_lsda(enc, exp)		.cfi_lsda enc, exp

/* Syntactic details of assembler.  */

/* This macro is for setting proper CFI with DW_CFA_expression describing
   the register as saved relative to %rsp instead of relative to the CFA.
   Expression is DW_OP_drop, DW_OP_breg7 (%rsp is register 7), sleb128 offset
   from %rsp.  */
#define cfi_offset_rel_rsp(regn, off)	.cfi_escape 0x10, regn, 0x4, 0x13, \
					0x77, off & 0x7F | 0x80, off >> 7

/* ELF uses byte-counts for .align, most others use log2 of count of bytes.  */
#define ALIGNARG(log2) 1<<log2
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name;


/* Define an entry point visible from C.  */
#define	ENTRY(name)							      \
  .globl C_SYMBOL_NAME(name);						      \
  .type C_SYMBOL_NAME(name),@function;					      \
  .align ALIGNARG(4);							      \
  C_LABEL(name)								      \
  cfi_startproc;							      \
  CALL_MCOUNT

#undef	END
#define END(name)							      \
  cfi_endproc;								      \
  ASM_SIZE_DIRECTIVE(name)

#define ENTRY_CHK(name) ENTRY (name)
#define END_CHK(name) END (name)

/* If compiled for profiling, call `mcount' at the start of each function.  */
#ifdef	PROF
/* The mcount code relies on a normal frame pointer being on the stack
   to locate our caller, so push one just for its benefit.  */
#define CALL_MCOUNT                                                          \
  pushq %rbp;                                                                \
  cfi_adjust_cfa_offset(8);                                                  \
  movq %rsp, %rbp;                                                           \
  cfi_def_cfa_register(%rbp);                                                \
  call JUMPTARGET(mcount);                                                   \
  popq %rbp;                                                                 \
  cfi_def_cfa(rsp,8);
#else
#define CALL_MCOUNT		/* Do nothing.  */
#endif

/* Since C identifiers are not normally prefixed with an underscore
   on this system, the asm identifier `syscall_error' intrudes on the
   C name space.  Make sure we use an innocuous name.  */
#define	syscall_error	__syscall_error
#define mcount		_mcount

#define	PSEUDO(name, syscall_name, args)				      \
lose:									      \
  jmp JUMPTARGET(syscall_error)						      \
  .globl syscall_error;							      \
  ENTRY (name)								      \
  DO_CALL (syscall_name, args);						      \
  jb lose

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  END (name)

#undef JUMPTARGET
#ifdef PIC
#define JUMPTARGET(name)	name##@PLT
#else
#define JUMPTARGET(name)	name
#endif

/* Local label name for asm code. */
#ifndef L
/* ELF-like local names start with `.L'.  */
# define L(name)	.L##name
#endif

#define atom_text_section .section ".text.atom", "ax"

/* Long and pointer size in bytes.  */
#define LP_SIZE	8

/* Instruction to operate on long and pointer.  */
#define LP_OP(insn) insn##q

/* Assembler address directive. */
#define ASM_ADDR .quad

/* Registers to hold long and pointer.  */
#define RAX_LP	rax
#define RBP_LP	rbp
#define RBX_LP	rbx
#define RCX_LP	rcx
#define RDI_LP	rdi
#define RDX_LP	rdx
#define RSI_LP	rsi
#define RSP_LP	rsp
#define R8_LP	r8
#define R9_LP	r9
#define R10_LP	r10
#define R11_LP	r11
#define R12_LP	r12
#define R13_LP	r13
#define R14_LP	r14
#define R15_LP	r15

#else	/* __ASSEMBLER__ */

/* Long and pointer size in bytes.  */
#define LP_SIZE "8"

/* Instruction to operate on long and pointer.  */
#define LP_OP(insn) #insn "q"

/* Assembler address directive. */
#define ASM_ADDR ".quad"

/* Registers to hold long and pointer.  */
#define RAX_LP	"rax"
#define RBP_LP	"rbp"
#define RBX_LP	"rbx"
#define RCX_LP	"rcx"
#define RDI_LP	"rdi"
#define RDX_LP	"rdx"
#define RSI_LP	"rsi"
#define RSP_LP	"rsp"
#define R8_LP	"r8"
#define R9_LP	"r9"
#define R10_LP	"r10"
#define R11_LP	"r11"
#define R12_LP	"r12"
#define R13_LP	"r13"
#define R14_LP	"r14"
#define R15_LP	"r15"

#endif	/* __ASSEMBLER__ */

#endif	/* _X86_64_SYSDEP_H */
