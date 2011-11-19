/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>

// Lab 4c challenge
#include <kern/myipc.h>

// Lab 6
#include <kern/e100.h>
#include <inc/ns.h>
// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.
	
	// LAB 3: Your code here.

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);

	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	int ret;
	struct Env *newenv;

	// cprintf("%d Exofork a child\n", curenv->env_id);
	if ((ret = env_alloc(&newenv, curenv->env_id)) < 0){
		return ret;
	}
	// cprintf("Child %d created\n", newenv->env_id);
	// set the status to be ENV_NOT_RUNNABLE
	newenv->env_status = ENV_NOT_RUNNABLE;

	// register set
	newenv->env_tf = curenv->env_tf;
	newenv->env_tf.tf_regs.reg_eax = 0;
	
	return newenv->env_id;
	// panic("sys_exofork not implemented");
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	// panic("sys_env_set_status not implemented");
	struct Env *env;

	if (envid2env(envid, &env, 1) != 0){
		return -E_BAD_ENV;
	}

	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE){
		return -E_INVAL;
	}

	env->env_status = status;
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env *env;
	pte_t *entry;

	if (envid2env(envid, &env, 1) != 0){
		return -E_BAD_ENV;
	}

	entry = pgdir_walk(curenv->env_pgdir, (void *)tf, 0);
	if (entry == NULL){
		return -E_INVAL;
	}

	env->env_tf = *tf;
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	// panic("sys_env_set_pgfault_upcall not implemented");
	struct Env *env;
	// cprintf("sys_env_set_pgfault_upcall\n");
	if (envid2env(envid, &env, 1) != 0){
		return -E_BAD_ENV;
	}
	env->env_pgfault_upcall = NULL;
	// I haven't checked the permission for the caller, is curenv != env
	// if (env != curenv){ return -E_BAD_ENV; }

	// cprintf("sys_env_set_pgfault_upcall\n");
	env->env_pgfault_upcall = func;
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_USER in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	struct Env *env = NULL;
	struct Page *pp = NULL;
	int allowed_perm = 0;
	
	// Check envid and get env
	if (envid2env(envid, &env, 1) != 0){
		return -E_BAD_ENV;
	}

	// Check va
	if ((uint32_t)va >= UTOP || (uint32_t)va % PGSIZE != 0){
		return -E_INVAL;
	}

	// Check perm
	allowed_perm = PTE_U | PTE_P;
	if ((perm & allowed_perm) != allowed_perm){
		return -E_INVAL;
	}
	allowed_perm |= PTE_W | PTE_AVAIL;
	// Check if there are other permissions
	if ((perm ^ (perm & allowed_perm)) != 0){
		return -E_INVAL;
	}
	// cprintf("sys_page_alloc **********\n");
	// alloc a physical page
	if (page_alloc(&pp) != 0){
		return -E_NO_MEM;
	}
	memset(page2kva(pp), 0, PGSIZE);
	// pp->pp_ref = 1;

	// insert into the page table of env
	if (page_insert(env->env_pgdir, pp, va, perm | PTE_P) != 0){
		page_free(pp);
		return -E_NO_MEM;
	}

	return 0;
	// panic("sys_page_alloc not implemented");
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	struct Env *srcenv, *dstenv;
	pte_t *srcentry, *dstentry;
	int allowed_perm;

	// Get env
	if (envid2env(srcenvid, &srcenv, 0) != 0){
		return -E_BAD_ENV;
	}
	if (envid2env(dstenvid, &dstenv, 0) != 0){
		return -E_BAD_ENV;
	}
	// Debug
	// cprintf("sys_page_map *** src %x dst %x\n", srcenv->env_id, dstenv->env_id);

	// check the validity of srcva, dstva
	if ((uint32_t)srcva >= UTOP || (uint32_t)dstva >= UTOP){
		return -E_INVAL;
	}
	if ((uint32_t)srcva % PGSIZE != 0 || (uint32_t)dstva % PGSIZE != 0){
		return -E_INVAL;
	}

	srcentry = pgdir_walk(srcenv->env_pgdir, srcva, 0);
	if (srcentry == NULL){
		return -E_INVAL;
	}

	// Check perm
	allowed_perm = PTE_U | PTE_P;
	if ((perm & allowed_perm) != allowed_perm){
		return -E_INVAL;
	}

	allowed_perm |= PTE_W | PTE_AVAIL;
	// Check if there are other permissions
	if ((perm ^ (perm & allowed_perm)) != 0){
		return -E_INVAL;
	}
	if (((*srcentry) & PTE_W) == 0 && (perm & PTE_W)){
		return -E_INVAL;
	}

	/*dstentry = pgdir_walk(dstenv->env_pgdir, dstva, 1);
	if (dstentry == NULL){
		return -E_NO_MEM;
	}*/
	if (page_insert(dstenv->env_pgdir, pa2page(PTE_ADDR(*srcentry)), dstva, perm) < 0){
		return -E_NO_MEM;
	}
	return 0;
	// panic("sys_page_map not implemented");
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	struct Env *env;
	pte_t *entry;

	if (envid2env(envid, &env, 1) != 0){
		return -E_BAD_ENV;
	}
	// the caller doesn't have permission to change envid?
	
	if ((uint32_t)va >= UTOP || (uint32_t)va % PGSIZE != 0){
		return -E_INVAL;
	}

	page_remove(env->env_pgdir, va);

	return 0;
	// panic("sys_page_unmap not implemented");
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	int r;
	int allowed_perm;
	struct Env *env;
	pte_t *entry;
	
	// Debug info
	// cprintf("%x calls sys_ipc_try_send to %x\n", sys_getenvid(), envid);
	// Check envid
	if ((r = envid2env(envid, &env, 0)) < 0){
		return -E_BAD_ENV;
	}
	// cprintf("env is good\n");
	// Target not blocked, waiting for ipc
	if (env->env_status == ENV_RUNNABLE && env->env_ipc_recving){
		return -E_IPC_NOT_RECV;
	}

	// Check srcva
	if ((uint32_t)srcva < UTOP){
		if ((uint32_t)srcva % PGSIZE != 0){
			return -E_INVAL;
		}	
		// check perm
		allowed_perm = PTE_U | PTE_P;
		if ((perm & allowed_perm) != allowed_perm){
			return -E_INVAL;
		}
		allowed_perm |= PTE_W | PTE_AVAIL;
		// Check if there are other permissions
		if ((perm ^ (perm & allowed_perm)) != 0){
			return -E_INVAL;
		}

		// Get mapped entry of srcva
		entry = pgdir_walk(curenv->env_pgdir, srcva, 0);
		if (entry == NULL){
			return -E_INVAL;
		}

		// Check read-only
		if ((perm & PTE_W) && (*entry & PTE_W) == 0){
			return -E_INVAL;
		}
	}

	// Env is not blocked for ipc, add the message into kernel's message queue
	if (env->env_status == ENV_RUNNABLE && env->env_ipc_recving == 0){
		// add to the message queue
		if (myipc_queue_push(env->env_id, curenv->env_id, value, srcva, perm) < 0){
			return -E_IPC_NOT_RECV;
		}
		// curenv is blocked
		curenv->env_status = ENV_NOT_RUNNABLE;
		// return -E_IPC_NOT_RECV;
		return 0;
	}
	// Debug info
	// cprintf("%x sys_ipc_try_send: target %x blocked, we can go on\n", sys_getenvid(), env->env_id);

	// send succeeds
	env->env_ipc_recving = 0;
	env->env_ipc_from = sys_getenvid();
	env->env_ipc_value = value;
	env->env_status = ENV_RUNNABLE;

	if ((uint32_t)srcva < UTOP){
		//env->env_ipc_dstva = srcva;
		if ((uint32_t)env->env_ipc_dstva < UTOP){
			if ((r = sys_page_map(0, srcva, envid, env->env_ipc_dstva, perm)) < 0){
				return r;
			}
			env->env_ipc_perm = perm;
		} else {
			env->env_ipc_perm = 0;
		}
	} else {
		env->env_ipc_perm = 0;
	}

	// Debug info
	// cprintf("%x sys_ipc_try_send succeeds\n", sys_getenvid());
	// panic("sys_ipc_try_send not implemented");
	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	// cprintf("%08x calls sys_ipc_recv\n", sys_getenvid());

	// Challenge 
	int r;
	pte_t *entry;
	struct Myipc *message;
	struct Env *srcenv, *dstenv;
	
	message = myipc_queue_pop(sys_getenvid());
	if (message != NULL){
		/* if ((r = envid2env(message->ipc_to, &dstenv, 0)) < 0){
			return -E_BAD_ENV;
		}*/
		// cprintf("Message %x\n", message->ipc_from);
		if ((r = envid2env(message->ipc_from, &srcenv, 0)) < 0){
			return -E_BAD_ENV;
		}
		// send succeeds
		curenv->env_ipc_recving = 0;
		curenv->env_ipc_from = srcenv->env_id;
		curenv->env_ipc_value = message->ipc_value;
		// dstenv->env_status = ENV_RUNNABLE;

		if ((uint32_t)dstva < UTOP){
			//env->env_ipc_dstva = srcva;
			if ((uint32_t)message->ipc_va < UTOP){
				sys_page_map(message->ipc_from, message->ipc_va, 0, dstva, message->ipc_perm);
				curenv->env_ipc_perm = message->ipc_perm;
			} else {
				curenv->env_ipc_perm = 0;
			}
		} else {
			curenv->env_ipc_perm = 0;
		}
		srcenv->env_status = ENV_RUNNABLE;
		// Debug info
		return 0;
	}
	// cprintf("*** No message ***\n");
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_ipc_recving = 1;

	if ((uint32_t)dstva < UTOP){
		if ((uint32_t)dstva % PGSIZE != 0){
			curenv->env_status = ENV_RUNNABLE;
			curenv->env_ipc_recving = 0;
			return -E_INVAL;
		}
		curenv->env_ipc_dstva = dstva;
	} else {
		curenv->env_ipc_dstva = 0;
	}

	// sched_yield();
	// panic("sys_ipc_recv not implemented");
	return 0;
}

// Return the current time.
static int
sys_time_msec(void) 
{
	// LAB 6: Your code here.
	return time_msec();
	// panic("sys_time_msec not implemented");
}

// Return number of data sent
int
sys_net_send(void *buf, size_t size)
{ 
	return nic_send_packet(buf, size);
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	
	// Debug info
	// cprintf("Syscallno %x: %x %x %x %x %x\n", syscallno, a1, a2, a3, a4, a5);

	switch(syscallno){
		case SYS_cputs:
			user_mem_assert(curenv, (void *)a1, a2, PTE_U);
			sys_cputs((char *)a1, a2);
			return 0;
			break;
		case SYS_cgetc:
			return sys_cgetc();
			break;
		case SYS_getenvid:
			return sys_getenvid();
			break;
		case SYS_env_destroy:
			return sys_env_destroy(a1);
			break;
		case SYS_page_alloc:
			return sys_page_alloc(a1, (void *)a2, a3);
			break;
		case SYS_page_map:
			return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);
			break;
		case SYS_page_unmap:
			return sys_page_unmap(a1, (void *)a2);
			break;
		case SYS_exofork:
			return sys_exofork();
			break;
		case SYS_env_set_status:
			return sys_env_set_status(a1, a2);
			break;
		case SYS_env_set_trapframe:
			return sys_env_set_trapframe(a1, (struct Trapframe *)a2);
			break;
		// set page fault upcall
		case SYS_env_set_pgfault_upcall:
			return sys_env_set_pgfault_upcall(a1, (void *)a2);
			break;
		// scheduling
		case SYS_yield:
			sys_yield();
		case SYS_ipc_try_send:
			return sys_ipc_try_send(a1, a2, (void *)a3, a4);
			break;
		case SYS_ipc_recv:
			return sys_ipc_recv((void *)a1);
			break;
		case SYS_time_msec:
			return sys_time_msec();
			break;
		case SYS_net_send:
			return sys_net_send((void *)a1, a2);
			break;
		case NSYSCALLS:
		default:
			return -E_INVAL;
			break;
	}

	// panic("syscall not implemented");
}

