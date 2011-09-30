#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>

static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	return "(unknown trap)";
}

// interrupt handler function declaration
extern void trap_divide(struct Trapframe *);
extern void trap_debug(struct Trapframe *);
extern void trap_nmi(struct Trapframe *);
extern void trap_brkpt(struct Trapframe *);
extern void trap_oflow(struct Trapframe *);
extern void trap_bound(struct Trapframe *);
extern void trap_illop(struct Trapframe *);
extern void trap_device(struct Trapframe *);
extern void trap_dblflt(struct Trapframe *);
extern void trap_tss(struct Trapframe *);
extern void trap_segnp(struct Trapframe *);
extern void trap_stack(struct Trapframe *);
extern void trap_gpflt(struct Trapframe *);
extern void trap_pgflt(struct Trapframe *);
extern void trap_fperr(struct Trapframe *);
extern void trap_align(struct Trapframe *);
extern void trap_mchk(struct Trapframe *);
extern void trap_simderr(struct Trapframe *);
extern void trap_syscall(struct Trapframe *);

extern int rambo;

void
idt_init(void)
{
	extern struct Segdesc gdt[];
	
	// LAB 3: Your code here.
	// SETGATE(gate, istrap, sel, off, dpl)

	// Debug info
	// cprintf("Rambo %d\n", rambo);

	SETGATE(idt[T_DIVIDE], 0, GD_KT, trap_divide, 0);
	SETGATE(idt[T_DEBUG], 0, GD_KT, trap_debug, 3);
	SETGATE(idt[T_NMI], 0, GD_KT, trap_nmi, 0);
	SETGATE(idt[T_BRKPT], 0, GD_KT, trap_brkpt, 0);
	SETGATE(idt[T_OFLOW], 0, GD_KT, trap_oflow, 0);
	SETGATE(idt[T_BOUND], 0, GD_KT, trap_bound, 0);
	SETGATE(idt[T_ILLOP], 0, GD_KT, trap_illop, 0);
	SETGATE(idt[T_DEVICE], 0, GD_KT, trap_device, 0);
	SETGATE(idt[T_DBLFLT], 0, GD_KT, trap_dblflt, 0);
	SETGATE(idt[T_TSS], 0, GD_KT, trap_tss, 0);
	SETGATE(idt[T_SEGNP], 0, GD_KT, trap_segnp, 0);
	SETGATE(idt[T_STACK], 0, GD_KT, trap_stack, 0);
	SETGATE(idt[T_GPFLT], 0, GD_KT, trap_gpflt, 0);
	SETGATE(idt[T_PGFLT], 0, GD_KT, trap_pgflt, 0);
	SETGATE(idt[T_FPERR], 0, GD_KT, trap_fperr, 0);
	SETGATE(idt[T_ALIGN], 0, GD_KT, trap_align, 0);
	SETGATE(idt[T_MCHK], 0, GD_KT, trap_mchk, 0);
	SETGATE(idt[T_SIMDERR], 0, GD_KT, trap_simderr, 0);

	SETGATE(idt[T_SYSCALL], 0, GD_KD, trap_syscall, 0);


	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

	// Load the IDT
	asm volatile("lidt idt_pd");
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.

	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	cprintf("Incoming TRAP frame at %p\n", tf);

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		assert(curenv);
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}
	
	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// Return to the current environment, which should be runnable.
	assert(curenv && curenv->env_status == ENV_RUNNABLE);
	env_run(curenv);
}

// interrupt and trap handlers

// Divide error fault handler
void
divide_error_handler(struct Trapframe *tf)
{
	cprintf("TRAP frame at %08x\n", (uint32_t)tf);
	cprintf("  trap %08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	cprintf("  eip  %08x\n", tf->tf_eip);
	cprintf("  ss   %08x\n", tf->tf_ss);

}

void
debug_exception_handler(struct Trapframe *tf)
{
}

void
nonmaskable_interrupt_handler(struct Trapframe *tf)
{
}

void
breakpoint_handler(struct Trapframe *tf)
{
}

void
overflow_handler(struct Trapframe *tf)
{
}

void
bounds_check_handler(struct Trapframe *tf)
{
}

void 
illegal_opcode_handler(struct Trapframe *tf)
{
}

void 
device_not_available_handler(struct Trapframe *tf)
{
}

void
double_fault_handler(struct Trapframe *tf)
{
}

void
invalid_task_switch_segment_handler(struct Trapframe *tf)
{
}

void
segment_not_present_handler(struct Trapframe *tf)
{
}

void
stack_exception_handler(struct Trapframe *tf)
{
}

void
general_protection_fault_handler(struct Trapframe *tf)
{
}

void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.
	
	// LAB 3: Your code here.

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

void
floating_point_error_handler(struct Trapframe *tf)
{
}

void
aligment_check_handler(struct Trapframe *tf)
{
}

void
machine_check_handler(struct Trapframe *tf)
{
}

void
simd_floating_point_error_handler(struct Trapframe *tf)
{
}

void
system_call_handler(struct Trapframe *tf)
{
}
