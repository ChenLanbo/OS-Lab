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
		cprintf("Usage: showmappings virtual_address ...\n");
		cprintf("    virtual_address must be hexdecimal format like 0x00000000\n");
		return 1;
	}

	for (i = 1; i < argc; i++){
		if (atoh(argv[i], &va) == 1){ //&& atoi(argv[i], &va) == 1)
			cprintf("Error address format for argument %d: %s\n", argv[i], i);
			return 1;
		}

		pte_t *entry = pgdir_walk(boot_pgdir, (void *)va, 0);
		if (entry == NULL){
			cprintf("0x%08x: currently has no mapping\n\n", va);
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

// mapping_chmod: change the permissions of the mapping 
// 				  in the current address space
int
mon_mapping_chmod(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 1 || argc == 2 || argc > 4){
		cprintf("Usage: mapping_chmod [+Perm] [-Perm] address\n");
		cprintf("  +Perm  add permission to the page mapping\n");
		cprintf("  -Perm  remove permission from the page mapping\n");
		cprintf("   Perm  w    --- read and write\n");
		cprintf("         u    --- user level access\n");
		cprintf("         pwt  --- page level write through\n");
		cprintf("         pcd  --- page level cache disable\n");
		cprintf("         a    --- accessed\n");
		cprintf("         d    --- dirty\n");
		cprintf("         mbz  --- bits must be zero\n");
		cprintf(" address hexdecimal format like 0xffffffff\n");
		return 1;
	}

	uint32_t perm_on = 0, perm_off = 0, address;
	pte_t *entry;

	if (argc == 3){
		if (argv[1][0] == '+'){

			if (strcmp(argv[1]+1, "w") == 0){
				perm_on = PTE_W;
			} else if (strcmp(argv[1]+1, "u") == 0){
				perm_on = PTE_U;
			} else if (strcmp(argv[1]+1, "pwt") == 0){
				perm_on = PTE_PWT;
			} else if (strcmp(argv[1]+1, "pcd") == 0){
				perm_on = PTE_PCD;
			} else if (strcmp(argv[1]+1, "a") == 0){
				perm_on = PTE_A;
			} else if (strcmp(argv[1]+1, "d") == 0){
				perm_on = PTE_D;
			} else if (strcmp(argv[1]+1, "mbz") == 0){
				perm_on = PTE_MBZ;
			} else {
				cprintf("Error permission\n");
				return 1;
			}

		} else if (argv[1][0] == '-'){

			if (strcmp(argv[1]+1, "w") == 0){
				perm_off = PTE_W;
			} else if (strcmp(argv[1]+1, "u") == 0){
				perm_off = PTE_U;
			} else if (strcmp(argv[1]+1, "pwt") == 0){
				perm_off = PTE_PWT;
			} else if (strcmp(argv[1]+1, "pcd") == 0){
				perm_off = PTE_PCD;
			} else if (strcmp(argv[1]+1, "a") == 0){
				perm_off = PTE_A;
			} else if (strcmp(argv[1]+1, "d") == 0){
				perm_off = PTE_D;
			} else if (strcmp(argv[1]+1, "mbz") == 0){
				perm_off = PTE_MBZ;
			} else {
				cprintf("Error permission\n");
				return 1;
			}

		} else {
			cprintf("Error option\n");
			return 1;
		}

		if (atoh(argv[2], &address) == 1){
			cprintf("Error address format\n");
			return 1;
		}

	} else if (argc == 4){

		if (argv[1][0] == '+'){

			if (strcmp(argv[1]+1, "w") == 0){
				perm_on = PTE_W;
			} else if (strcmp(argv[1]+1, "u") == 0){
				perm_on = PTE_U;
			} else if (strcmp(argv[1]+1, "pwt") == 0){
				perm_on = PTE_PWT;
			} else if (strcmp(argv[1]+1, "pcd") == 0){
				perm_on = PTE_PCD;
			} else if (strcmp(argv[1]+1, "a") == 0){
				perm_on = PTE_A;
			} else if (strcmp(argv[1]+1, "d") == 0){
				perm_on = PTE_D;
			} else if (strcmp(argv[1]+1, "mbz") == 0){
				perm_on = PTE_MBZ;
			} else {
				cprintf("Error permission\n");
				return 1;
			}

		} else {
			cprintf("No add option\n");
			return 1;
		}

		if (argv[2][0] == '-'){

			if (strcmp(argv[1]+1, "w") == 0){
				perm_off = PTE_W;
			} else if (strcmp(argv[1]+1, "u") == 0){
				perm_off = PTE_U;
			} else if (strcmp(argv[1]+1, "pwt") == 0){
				perm_off = PTE_PWT;
			} else if (strcmp(argv[1]+1, "pcd") == 0){
				perm_off = PTE_PCD;
			} else if (strcmp(argv[1]+1, "a") == 0){
				perm_off = PTE_A;
			} else if (strcmp(argv[1]+1, "d") == 0){
				perm_off = PTE_D;
			} else if (strcmp(argv[1]+1, "mbz") == 0){
				perm_off = PTE_MBZ;
			} else {
				cprintf("Error permission\n");
				return 1;
			}

		} else {
			cprintf("No remove option\n");
			return 1;
		}

		if (atoh(argv[3], &address) == 1){
			cprintf("Error address format\n");
			return 1;
		}
	}

	entry = pgdir_walk(boot_pgdir, (void *)address, 0);

	if (entry == NULL){
		cprintf("Virtual address 0x%08x currently is not mapped to any page\n", address);
		return 1;
	}

	*entry |= perm_on;

	if (*entry & perm_off){
		(*entry) ^= perm_off;
	}

	return 0;
}

// memdump: dump the memory contents
int 
mon_memdump(int argc, char **argv, struct Trapframe *tf)
{
	// arguments check
	if (argc == 1 || argc > 4){
		cprintf("Usage: memdump [-p] [range] address\n");
		cprintf("            -p  dump the physical memory contents\n");
		cprintf("         range  the range of memory in bytes start from address\n");
		cprintf("       address  starting address in hexdecimal format like 0x00000000\n");
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
			cprintf("Error address format. It must be hexdecimal like 0x00000000\n");
			return 1;
		}
	} else if (argc == 3){
		if (strcmp(argv[1], "-p") == 0){
			physical = 1;
		} else if (atoi(argv[1], (uint32_t *)&range) == 1 && atoh(argv[1], (uint32_t *)&range) == 1){
			cprintf("Unknown number format\n");
			return 1;
		}
		if (atoh(argv[2], &address) == 1){
			cprintf("Error address format. It must be hexdecimal\n");
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
			cprintf("Error address format. It must be hexdecimal\n");
			return 1;
		}
	}

	cprintf("       Memdump:\n");
	for (i = 0; i < range; i += 4){
		cprintf("    0x%08x:", address + i);
		for (j = 0; j < 4; j++){
			// physical address option is set
			if (physical){

				if ((physaddr_t)(address + i + j) >= max_physaddr){
					a = NULL;
				} else {
					a = (void *)KADDR(address + i + j);
				}

			} else {

				entry = pgdir_walk(boot_pgdir, (void *)(address + i + j), 0);

				if (entry == NULL){
					a = NULL;
				} else {
					// PTE_PS bit is set
					if (*entry & PTE_PS){
						a = KADDR((PDX(*entry) << PDXSHIFT) | ((address + i + j) & 0x3fffff));
					} else {
						a = KADDR((PTE_ADDR(*entry) | ((address + i + j) & 0xfff)));
					}

				}
			}

			// output
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

	if (argc != 1){
		cprintf("Usage: alloc_page\n");
		return 1;
	}

	if (page_alloc(&pp) != 0){
		cprintf("No free memory available\n");
		return 1;
	}

	pp->pp_ref = 1;
	LIST_INSERT_HEAD(&allocated_page_list, pp, pp_link);

	cprintf("    New page: 0x%x\n", page2pa(pp));
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
			cprintf("    No pages allocated by alloc_page currently\n");
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
