/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

	/* divide error */
	TRAPHANDLER_NOEC(trap_divide, T_DIVIDE) 

	/* debug */
	TRAPHANDLER_NOEC(trap_debug, T_DEBUG)

	/* non-maskable interrupt */
	TRAPHANDLER_NOEC(trap_nmi, T_NMI)

	/* breakpoint */
	TRAPHANDLER_NOEC(trap_brkpt, T_BRKPT)

	/* overflow */
	TRAPHANDLER_NOEC(trap_oflow, T_OFLOW)

	/* bounds check */
	TRAPHANDLER_NOEC(trap_bound, T_BOUND)

	/* illegal opcode */
	TRAPHANDLER_NOEC(trap_illop, T_ILLOP)

	/* device not available */
	TRAPHANDLER_NOEC(trap_device, T_DEVICE)

	/* Error code: 0, double fault*/
	TRAPHANDLER(trap_dblflt, T_DBLFLT)

	/* Error code: segment selector index, invalid task switch segment*/
	TRAPHANDLER(trap_tss, T_TSS)

	/* Error code: segment selector index, segment not present*/
	TRAPHANDLER(trap_segnp, T_SEGNP)

	/* stack */
	TRAPHANDLER(trap_stack, T_STACK)

	/* general protection fault */
	TRAPHANDLER(trap_gpflt, T_GPFLT)

	/* page fault */
	TRAPHANDLER(trap_pgflt, T_PGFLT)

	/* floating point error */
	TRAPHANDLER_NOEC(trap_fperr, T_FPERR)

	/* alignment check */
	TRAPHANDLER_NOEC(trap_align, T_ALIGN)

	/* machine check */
	TRAPHANDLER_NOEC(trap_mchk, T_MCHK)

	/* SIMD floating point error */
	TRAPHANDLER_NOEC(trap_simderr, T_SIMDERR)

	/* syscall */
	TRAPHANDLER_NOEC(trap_syscall, T_SYSCALL)

	/*  External interrupt(IRQs)  */
	TRAPHANDLER_NOEC(trap_iqr_0, IRQ_OFFSET + 0x0)
	TRAPHANDLER_NOEC(trap_iqr_1, IRQ_OFFSET + 0x1)
	TRAPHANDLER_NOEC(trap_iqr_2, IRQ_OFFSET + 0x2)
	TRAPHANDLER_NOEC(trap_iqr_3, IRQ_OFFSET + 0x3)
	TRAPHANDLER_NOEC(trap_iqr_4, IRQ_OFFSET + 0x4)
	TRAPHANDLER_NOEC(trap_iqr_5, IRQ_OFFSET + 0x5)
	TRAPHANDLER_NOEC(trap_iqr_6, IRQ_OFFSET + 0x6)
	TRAPHANDLER_NOEC(trap_iqr_7, IRQ_OFFSET + 0x7)
	TRAPHANDLER_NOEC(trap_iqr_8, IRQ_OFFSET + 0x8)
	TRAPHANDLER_NOEC(trap_iqr_9, IRQ_OFFSET + 0x9)
	TRAPHANDLER_NOEC(trap_iqr_a, IRQ_OFFSET + 0xa)
	TRAPHANDLER_NOEC(trap_iqr_b, IRQ_OFFSET + 0xb)
	TRAPHANDLER_NOEC(trap_iqr_c, IRQ_OFFSET + 0xc)
	TRAPHANDLER_NOEC(trap_iqr_d, IRQ_OFFSET + 0xd)
	TRAPHANDLER_NOEC(trap_iqr_e, IRQ_OFFSET + 0xe)
	TRAPHANDLER_NOEC(trap_iqr_f, IRQ_OFFSET + 0xf)

/*
 * Lab 3: Your code here for _alltraps
 *        The cpu has pushed old SS, old ESP, old, EFLAGS, old cs, old EIP onto the stack
 *        Do I call IRET to return ?
 */

_alltraps:

	pushw $0
	pushw %ds
	pushw $0
	pushw %es
	pushal

	/* clear IF flag to disable IRQs */
	cli

	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es

	pushl %esp

	call trap

