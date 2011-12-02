#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/time.h>

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
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
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

// external interrupts(IRQs)
extern void trap_irq_0(struct Trapframe *);
extern void trap_irq_1(struct Trapframe *);
extern void trap_irq_2(struct Trapframe *);
extern void trap_irq_3(struct Trapframe *);
extern void trap_irq_4(struct Trapframe *);
extern void trap_irq_5(struct Trapframe *);
extern void trap_irq_6(struct Trapframe *);
extern void trap_irq_7(struct Trapframe *);
extern void trap_irq_8(struct Trapframe *);
extern void trap_irq_9(struct Trapframe *);
extern void trap_irq_a(struct Trapframe *);
extern void trap_irq_b(struct Trapframe *);
extern void trap_irq_c(struct Trapframe *);
extern void trap_irq_d(struct Trapframe *);
extern void trap_irq_e(struct Trapframe *);
extern void trap_irq_f(struct Trapframe *);


void
idt_init(void)
{
	extern struct Segdesc gdt[];
	
	// LAB 3: Your code here.
	// Exceptions and traps
	SETGATE(idt[T_DIVIDE], 0, GD_KT, trap_divide, 0);
	SETGATE(idt[T_DEBUG], 0, GD_KT, trap_debug, 3);
	SETGATE(idt[T_NMI], 0, GD_KT, trap_nmi, 0);
	SETGATE(idt[T_BRKPT], 0, GD_KT, trap_brkpt, 3);
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

	// External interrupts
	SETGATE(idt[IRQ_OFFSET + 0x0], 0, GD_KT, trap_irq_0, 0);
	SETGATE(idt[IRQ_OFFSET + 0x1], 0, GD_KT, trap_irq_1, 0);
	SETGATE(idt[IRQ_OFFSET + 0x2], 0, GD_KT, trap_irq_2, 0);
	SETGATE(idt[IRQ_OFFSET + 0x3], 0, GD_KT, trap_irq_3, 0);
	SETGATE(idt[IRQ_OFFSET + 0x4], 0, GD_KT, trap_irq_4, 0);
	SETGATE(idt[IRQ_OFFSET + 0x5], 0, GD_KT, trap_irq_5, 0);
	SETGATE(idt[IRQ_OFFSET + 0x6], 0, GD_KT, trap_irq_6, 0);
	SETGATE(idt[IRQ_OFFSET + 0x7], 0, GD_KT, trap_irq_7, 0);
	SETGATE(idt[IRQ_OFFSET + 0x8], 0, GD_KT, trap_irq_8, 0);
	SETGATE(idt[IRQ_OFFSET + 0x9], 0, GD_KT, trap_irq_9, 0);
	SETGATE(idt[IRQ_OFFSET + 0xa], 0, GD_KT, trap_irq_a, 0);
	SETGATE(idt[IRQ_OFFSET + 0xb], 0, GD_KT, trap_irq_b, 0);
	SETGATE(idt[IRQ_OFFSET + 0xc], 0, GD_KT, trap_irq_c, 0);
	SETGATE(idt[IRQ_OFFSET + 0xd], 0, GD_KT, trap_irq_d, 0);
	SETGATE(idt[IRQ_OFFSET + 0xe], 0, GD_KT, trap_irq_e, 0);
	SETGATE(idt[IRQ_OFFSET + 0xf], 0, GD_KT, trap_irq_f, 0);

	// syscall should be trap,
	// SETCALLGATE(idt[T_SYSCALL], GD_KT, trap_syscall, 0);
	SETGATE(idt[T_SYSCALL], 0, GD_KT, trap_syscall, 3);


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
	int32_t ret;
	// if (tf->tf_trapno != 48) cprintf("****** No. %d\n", tf->tf_trapno);
	// Handle clock interrupts.
	// LAB 4: Your code here.
	if (tf->tf_trapno == IRQ_OFFSET + 0){
		// cprintf("Timer interrupt\n");
		time_tick();
		sched_yield();
		return ;
	}

	// Add time tick increment to clock interrupts.
	// LAB 6: Your code here.

	// Add time_tick above sched_yield

	// Handle spurious interrupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
		cprintf("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}

	// LAB 7: Keyboard interface

	if (tf->tf_trapno == IRQ_OFFSET + 1){
		kbd_intr();
		return ;
	}

	if (tf->tf_trapno == IRQ_OFFSET + 4){
		serial_intr();
		return ;
	}

	if (tf->tf_trapno == T_DIVIDE || tf->tf_trapno == T_ILLOP || tf->tf_trapno == T_GPFLT){
		// cprintf("*************");
		// return ;
	}


	if (tf->tf_trapno == T_DEBUG){
		// Debug info
		// cprintf("*** trap %08x %s ***\n", tf->tf_trapno, trapname(tf->tf_trapno));
		// Invoke monitor
		monitor(tf);
		return ;
	}

	if (tf->tf_trapno == T_BRKPT){
		// Debug info
		// cprintf("*** trap %08x %s ***\n", tf->tf_trapno, trapname(tf->tf_trapno));
		// Invoke monitor
		monitor(tf);
		return ;
	}

	if (tf->tf_trapno == T_PGFLT){
		page_fault_handler(tf);
	}

	if (tf->tf_trapno == T_SYSCALL){
		ret = syscall(tf->tf_regs.reg_eax, 
				      tf->tf_regs.reg_edx, 
					  tf->tf_regs.reg_ecx,
					  tf->tf_regs.reg_ebx,
					  tf->tf_regs.reg_edi,
					  tf->tf_regs.reg_esi);
		tf->tf_regs.reg_eax = ret;
		return ;
	}

	// Handle keyboard and serial interrupts.
	// LAB 7: Your code here.

	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT){
		if (tf->tf_trapno == T_DEBUG){
			return ;
		}
		panic("unhandled trap in kernel");
	}
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
	// Debug info
	// if (tf->tf_trapno != 48 && tf->tf_trapno != 32 && tf->tf_trapno != 14) cprintf("Trapno %d\n", tf->tf_trapno);
	assert(!(read_eflags() & FL_IF));

	// cprintf("Incoming TRAP frame at %p\n", tf);

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

	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNABLE)
		env_run(curenv);
	else
		sched_yield();
}

// interrupt and trap handlers
void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.
	// previlage level = 0
	if ((tf->tf_cs & 3) == 0){
		panic("kernel page fault");
	}
	// LAB 3: Your code here.

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// The trap handler needs one word of scratch space at the top of the
	// trap-time stack in order to return.  In the non-recursive case, we
	// don't have to worry about this because the top of the regular user
	// stack is free.  In the recursive case, this means we have to leave
	// an extra word between the current top of the exception stack and
	// the new stack frame because the exception stack _is_ the trap-time
	// stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	// Note that the grade script assumes you will first check for the page
	// fault upcall and print the "user fault va" message below if there is
	// none.  The remaining three checks can be combined into a single test.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.
	// Now we are in kernel mode
	if (curenv->env_pgfault_upcall != NULL){
		struct UTrapframe *utf;

		// Check tf_esp is in UXSTACK
		// -4, scratch space to save eip return address
		if (tf->tf_esp >= UXSTACKTOP - PGSIZE && tf->tf_esp < UXSTACKTOP){
			utf = (struct UTrapframe *)(tf->tf_esp - sizeof(struct UTrapframe) - 4);
		} else {
			utf = (struct UTrapframe *)(UXSTACKTOP - sizeof(struct UTrapframe));
		}

		// Check permission
		user_mem_assert(curenv, (void *)utf, sizeof(struct UTrapframe), PTE_U | PTE_W);

		// dump Trapframe info to UTrapframe
		utf->utf_fault_va = fault_va;
		utf->utf_err = tf->tf_err;
		utf->utf_regs = tf->tf_regs;
		utf->utf_eip = tf->tf_eip;
		utf->utf_eflags = tf->tf_eflags;
		utf->utf_esp = tf->tf_esp;

		// set eip to env_pgfault_upcall
		curenv->env_tf.tf_eip = (uint32_t)curenv->env_pgfault_upcall;
		curenv->env_tf.tf_esp = (uint32_t)utf;

		// Debug info
		// cprintf("Dispatch to user-mode page fault handler: fault_va %08x\n", fault_va);
		// if (fault_va >= USTACKTOP - PGSIZE && fault_va < USTACKTOP) cprintf("pgfautl on stack\n");
		env_run(curenv);
	} else {
		cprintf("curenv->env_pgfault_upcall is NULL\n");
	}

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

