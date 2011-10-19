// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	void *pageaddr;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	// Debug info
	// cprintf("Page fault err %d addr %x\n", err, addr);
	// if ((uint32_t)addr < USTACKTOP && (uint32_t)addr >= USTACKTOP - PGSIZE){
	// 	cprintf("Fault on stack\n");
	// }
	if (!(err & FEC_WR)){
		panic("no write access");
	}
	if (!(vpt[VPN(addr)] & PTE_COW)){
		panic("non copy-on-write page");
	}
	if (!(vpt[VPN(addr)] & PTE_P)){
		panic("no page present");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(sys_getenvid(), (void *)PFTEMP, PTE_U | PTE_W | PTE_P)) < 0){
		panic("sys_page_alloc error %e", r);
	}
	pageaddr = ROUNDDOWN(addr, PGSIZE);
	memmove((void *)PFTEMP, pageaddr, PGSIZE);
	if ((r = sys_page_map(0, (void *)PFTEMP, 0, pageaddr, PTE_P | PTE_U | PTE_W)) < 0){
		panic("sys_page_map error %e", r);
	}

	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
// 
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	pte_t entry = vpt[pn];

	// LAB 4: Your code here.
	if ((entry & PTE_W) || (entry & PTE_COW)){
		// cprintf("copy-on-write dup %x\n", PTE_AVAIL);
		if ((r = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_U | PTE_P | PTE_COW)) < 0){
			panic("sys_page_map error %e", r);
		}
		// mapping the parenet its own page as PTE_COW
		// because of PTE_W, parent needs to remap again
		if ((r = sys_page_map(0, (void *)(pn * PGSIZE), 0, (void *)(pn * PGSIZE), PTE_U | PTE_P | PTE_COW)) < 0){
			panic("sys_page_map error %e", r);
		}
	} else {
		// cprintf("Not copy-on-write dup\n");
		if ((r = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_U | PTE_P)) < 0){
			panic("sys_page_map error %e", r);
		}
	}
	
	// panic("duppage not implemented");
	return 0;
}

static int
duppage2(envid_t envid, unsigned pn)
{
	int r, perm;
	// Debug
	perm = vpt[pn] & (PTE_U | PTE_P | PTE_W);

	if ((r = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), perm)) < 0){
		panic("sys_page_map error %e", r);
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
extern void _pgfault_upcall(void);

envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t child;
	uint32_t itr;
	int r;

	// install pgfault handler
	set_pgfault_handler(pgfault);

	child = sys_exofork();
	if (child < 0){
		return child;
	}
	if (child == 0){
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (itr = UTEXT; itr < UXSTACKTOP - PGSIZE; itr += PGSIZE){
		if (!(vpd[VPD(itr)] & PTE_P)){
			continue;
		}
		if (!(vpt[VPN(itr)] & PTE_P)){
			continue;
		}
		if (!(vpt[VPN(itr)] & PTE_U)){
			continue;
		}
		duppage(child, VPN(itr));
	}

	if ((r = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)) < 0){
		panic("sys_page_alloc error %e", r);
	}

	if ((r = sys_env_set_pgfault_upcall(child, _pgfault_upcall)) < 0){
		panic("sys_env_set_pgfault_upcall error %e", r);
	}

	if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0){
		panic("sys_env_set_status error %e", r);
	}

	return child;
}

// Challenge!
// only copy-on-write user stack
int
sfork(void)
{
	envid_t child;
	uint32_t itr;
	int r;

	// install pgfault handler
	set_pgfault_handler(pgfault);

	child = sys_exofork();
	if (child < 0){
		return child;
	}
	if (child == 0){
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (itr = UTEXT; itr < USTACKTOP - PGSIZE; itr += PGSIZE){
		if (!(vpd[VPD(itr)] & PTE_P)){
			continue;
		}
		if (!(vpt[VPN(itr)] & PTE_P)){
			continue;
		}
		if (!(vpt[VPN(itr)] & PTE_U)){
			continue;
		}
		duppage2(child, VPN(itr));
	}

	// only copy-on-write STACK
	duppage(child, VPN(USTACKTOP - PGSIZE));

	if ((r = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)) < 0){
		panic("sys_page_alloc error %e", r);
	}

	if ((r = sys_env_set_pgfault_upcall(child, _pgfault_upcall)) < 0){
		panic("sys_env_set_pgfault_upcall error %e", r);
	}

	if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0){
		panic("sys_env_set_status error %e", r);
	}

	return child;

	// panic("sfork not implemented");
	// return -E_INVAL;
}
