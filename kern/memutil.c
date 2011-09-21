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

// memdump: dump the memory contents
int 
mon_memdump(int argc, char **argv, struct Trapframe *tf)
{

	if (argc == 1 || argc > 4){
		cprintf("Usage: memdump [-p] [number] <address>\n");
		cprintf("         -p  dump the physical memory contents\n");
		cprintf("     number  the range of memory in bytes start from <address>\n");
		cprintf("    address  hexdecimal format\n");
		return 1;
	}

	pte_t *entry;
	int i, j, range = 4, physical = 0;
	uint32_t address;
	void *a;
	uint8_t ch; 

	if (argc == 2){
		if (strcmp(argv[1], "-p") == 0){
			cprintf("Missing address argument\n");
			return 1;
		}
		if (atoh(argv[1], &address) == 1){
			cprintf("Error address format, must be hexdecimal\n");
			return 1;
		}
	} else if (argc == 3){
		if (strcmp(argv[1], "-p") == 0){
			physical = 1;
		} else if (atoi(argv[1], (uint32_t *)&range) == 1){
			cprintf("Unknown number format\n");
			return 1;
		}
		if (atoh(argv[2], &address) == 1){
			cprintf("Error address format, must be hexdecimal\n");
			return 1;
		}

	} else if (argc == 4){
		if (strcmp(argv[1], "-p") == 0){
			physical = 1;
		} else {
			cprintf("Error arguments\n");
			return 1;
		}
		if (atoi(argv[2], (uint32_t *)&range) == 1){
			cprintf("Unknown number format\n");
			return 1;
		}
		if (atoh(argv[3], &address) == 1){
			cprintf("Error address format, must be hexdecimal\n");
			return 1;
		}
	}

	cprintf("       Memdump:\n");
	for (i = 0; i < range; i += 4){
		cprintf("    0x%08x:", address + i);
		for (j = 0; j < 4; j++){
			if (physical){
				a = (void *)KADDR(address + i + j);

				if ((physaddr_t)a >= max_physaddr){
					a = NULL;
				}
			} else {
				entry = pgdir_walk(boot_pgdir, (void *)(address + i + j), 0);

				if (entry == NULL){
					a = NULL;
				} else {
					if (*entry & PTE_PS){
						a = KADDR((PDX(*entry) << PDXSHIFT) | ((address + i + j) & 0x3fffff));
					} else {
						a = KADDR((PTE_ADDR(*entry) | ((address + i + j) & 0xfff)));
					}
				}
			}

			if (a != NULL){
				ch = *((char *)a);
				cprintf(" %02x", ch);
			} else {
				cprintf(" 00");
			}
		}
		cprintf("\n");
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
		return 1;
	}
	if (!(str[1] == 'x' || str[1] == 'X')){
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
