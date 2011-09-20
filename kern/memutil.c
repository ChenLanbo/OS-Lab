#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/memutil.h>
#include <kern/pmap.h>
#include <inc/string.h>

static int isInitialized = 0;
static struct Page_list allocated_page_list; // allocated page list by alloc_page command

// Char string of hex format
int atoh(char *str, uint32_t *res);
// Char string of decimal format
int atoi(char *str, uint32_t *res);

// General memory management utilities

// showmappings: show the physical page mapping of given virtual address
int 
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	int i, j;
	uint32_t va, len;

	if (argc == 1){
		cprintf("Usage: showmappings [virtual address]\n");
		return 1;
	}

	for (i = 1; i < argc; i++){
		if (atoh(argv[i], &va) == 1 && atoi(argv[i], &va) == 1){
			cprintf("Error address format: %s\n", argv[i]);
			continue;
		}
		pte_t *entry = pgdir_walk(boot_pgdir, (void *)va, 0);
		if (entry == NULL){
			cprintf("%08x: currently has no mapping\n", va);
			continue;
		}
		cprintf("0x%08x: mapped to physical page at address 0x%08x\n", va, PTE_ADDR(*entry));
		cprintf("     Flags:");
		if (*entry & PTE_W) cprintf(" PTE_W");
		if (*entry & PTE_U) cprintf(" PTE_U");
		if (*entry & PTE_PWT) cprintf(" PTE_PWT");
		if (*entry & PTE_PCD) cprintf(" PTE_PCD");
		if (*entry & PTE_A) cprintf(" PTE_A");
		if (*entry & PTE_D) cprintf(" PTE_D");
		if (*entry & PTE_PS) cprintf(" PTE_PS");
		if (*entry & PTE_MBZ) cprintf(" PTE_MBZ");
		cprintf("\n\n");
	}
	return 0;
}

int 
mon_memdump(int argc, char **argv, struct Trapframe *tf)
{

	if (argc == 1){
		cprintf("Usage: memdump [-p] [format] [unit] [number] <address>\n");
		cprintf("\t      -p dump the physical memory contents\n");
		cprintf("\t[format] -x hexdecimal\n");
		cprintf("\t         -u unsigned decimal\n");
		cprintf("\t         -o octal\n");
		cprintf("\t  [uint] -b dump in bytes\n");
		cprintf("\t         -h dump in half-words(two bytes)\n");
		cprintf("\t         -w dump in words(four bytes\n");
		cprintf("\t[number] the range of memory start from <address>\n");
		cprintf("\t<address> hexdecimal format\n");
		return 1;
	}
	int i, hasAddressInput = 0;
	int isPhys = 0;
	int format = 0;
	int unit = 0;
	int range = 0;
	uint32_t address = 0;

	for (i = 1; i < argc; i++){
		if (strcmp(argv[i], "-p") == 0){
			isPhys = 1;
		} else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "-o") == 0){
			if (strcmp(argv[i], "-x") == 0)
				format = 0;
			else if (strcmp(argv[i], "-u") == 0)
				format = 1;
			else if (strcmp(argv[i], "-o") == 0)
				format = 2;
		} else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-w") == 0){
			if (strcmp(argv[i], "-w") == 0)
				unit = 0;
			else if (strcmp(argv[i], "-h") == 0)
				unit = 1;
			else if (strcmp(argv[i], "-b") == 0)
				unit = 2;
		} else if (atoi(argv[i], (uint32_t *)&range) == 0){
			;
		} else if (atoh(argv[i], &address) == 0){
			hasAddressInput = 1;
		}
	}

	;
	if (hasAddressInput){
		;
		cprintf("\t<address> hexdecimal format\n");
	}

	return 0;
}


// alloc_page: allocate a physical page of 4KB
// return 0: allocation succeeded
//           also print out the physical address of the allocated page
// return 1: error
int 
mon_alloc_page(int argc, char **argv, struct Trapframe *tf)
{
	struct Page *pp;

	if (isInitialized == 0){
		isInitialized = 1;
		LIST_INIT(&allocated_page_list);
	}

	if (page_alloc(&pp) != 0){
		cprintf("No free memory available\n");
		return 1;
	}

	pp->pp_ref = 1;
	LIST_INSERT_HEAD(&allocated_page_list, pp, pp_link);

	cprintf("\tNew page: 0x%x\n", page2pa(pp));
	return 0;
}

// page_status: show the physical page status given page physical address
int
mon_page_status(int argc, char **argv, struct Trapframe *tf)
{
	struct Page *pp, *var;
	physaddr_t address;

	if (argc == 1){
		int empty = 1;
		LIST_FOREACH(var, &allocated_page_list, pp_link){
			empty = 0;
			cprintf("    Ref %2u allocated by alloc_page\n", var->pp_ref);
		}

		if (empty){
			cprintf("    No pages allocated by alloc_page currently");
		}
		return 0;
	}

	if (atoh(argv[1], &address) == 1 || (address & 0xFFF)){
		cprintf("    Error page address\n");
		return 1;
	}

	pp = pa2page(address);

	if (pp->pp_ref == 0){
		cprintf("    Ref %2u free\n", pp->pp_ref);
	} else {
		cprintf("    Ref %2u allocated ", pp->pp_ref);
		LIST_FOREACH(var, &allocated_page_list, pp_link){
			if (var == pp){
				break;
			}
		}
		if (var == NULL){
			cprintf("by kernel, cannot be freed\n");
		} else {
			cprintf("by alloc_page\n");
		}
	}

	return 0;
}

// free_page: free the allocated page given page physical address
// 0 argument: free all the allocated pages allocated by alloc_page command
// return 0: success
//        1: error
int
mon_free_page(int argc, char **argv, struct Trapframe *tf)
{
	struct Page *pp, *var = NULL;
	physaddr_t address;
	int inAlloc = 0;
	if (argc == 1){
		int empty = 1;

		while (!LIST_EMPTY(&allocated_page_list)){
			empty = 0;
			pp = LIST_FIRST(&allocated_page_list);
			LIST_REMOVE(LIST_FIRST(&allocated_page_list), pp_link);

			cprintf("    Free page: 0x%x\n", page2pa(pp));

			page_decref(pp);
		}

		if (empty){
			cprintf("    No allocated pages\n");
			return 1;
		}
		return 0;
	}

	if (atoh(argv[1], &address) == 1 || (address & 0xFFF)){
		cprintf("    Error page address\n");
		return 1;
	}

	pp = pa2page(address);

	LIST_FOREACH(var, &allocated_page_list, pp_link){
		if (var == pp){
			break;
		}
	}

	if (var == NULL){
		cprintf("    Cannot free page: 0x%x\n", address);
		if (pp->pp_ref == 0){
			cprintf("    This page is already in free list\n");
		} else {
			cprintf("    This page is used by the kernel\n");
		}
		return 1;
	}

	LIST_REMOVE(var, pp_link);
	cprintf("    Free page: 0x%x\n", page2pa(var));
	page_decref(var);

	return 0;
}

// utility functions

// atoh: convert hexdecimal string to unsigned number
// res : save the hexdecimal number
// return : 0 success
// 			1 error
int 
atoh(char *str, uint32_t *res)
{
	uint32_t len = strlen(str), i, j, v = 0;

	if (len < 3 || str == NULL){
		return 1;
	}
	if (str[0] != '0'){
		// cprintf("AA\n");
		return 1;
	}
	if (!(str[1] == 'x' || str[1] == 'X')){
		// cprintf("BB\n");
		return 1;
	}

	for (i = 2; i < len; i++){
		if (str[i] >= '0' && str[i] <= '9'){
			j = str[i] - '0';
		} else if (str[i] >= 'a' && str[i] <= 'f'){
			j = str[i] - 'a' + 10;
		} else if (str[i] >= 'A' && str[i] <= 'F'){
			j = str[i] - 'A' + 10;
		} else {
			return 1;
		}
		v = (v << 4) | j;
	}

	*res = v;
	return 0;
}


// atoi: convert decimal string to integer
// res : save result
// return : 0 success
// 			1 error
int 
atoi(char *str, uint32_t *res)
{
	uint32_t len = strlen(str), i;
	int v = -1, sign = 0;

	if (len < 1 || str == NULL){
		return 1;
	}

	if (str[0] == '-' || str[0] == '+'){
		if (str[0] == '-'){
			sign = 1;
		}
	} else if (str[0] >= '0' && str[0] <= '9'){
		v = str[0] - '0';
	} else {
		return 1;
	}

	for (i = 1; i < len; i++){
		if (str[0] >= '0' && str[0] <= '9'){
			v = v * 10 + str[0] - '0'; } else {
			return 1;
		}
	}

	if (v == -1){
		return 1;
	}

	if (sign){
		v = -v;
	}

	*res = v;
	return 0;
}
