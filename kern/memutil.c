
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/memutil.h>
#include <kern/pmap.h>
#include <inc/string.h>


int 
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	int i, j;
	uint32_t va, len;
	// for (i = 0; i < argc; i++){ cprintf("%s ", argv[i]); } cprintf("\n");

	if (argc == 1){
		cprintf("Usage: showmappings <virtual address>\n       virtual address must be hexdecimal\n");
		return 1;
	}

	for (i = 1; i < argc; i++){
		va = 0;
		len = strlen(argv[i]);
		if (len > 2 && argv[i][0] == '0' && (argv[i][1] == 'x' || argv[i][1] == 'X')){
			if (len > 10){
				cprintf("1Address out of range: %s\n", argv[i]);
				continue;
			}
			for (j = 2; j < len; j++){
				if (('0' <= argv[i][j] && argv[i][j] <= '9') 
				 || ('a' <= argv[i][j] && argv[i][j] <= 'f')
				 || ('A' <= argv[i][j] && argv[i][j] <= 'F')){
					continue;
				}
				break;
			}
			if (j < len){
				cprintf("2Error address format: %s\n", argv[i]);
				continue;
			}

			for (j = 2; j < len; j++){
				if ('0' <= argv[i][j] && argv[i][j] <= '9') va = va * 16 + (argv[i][j] - '0');
				if ('a' <= argv[i][j] && argv[i][j] <= 'f') va = va * 16 + (argv[i][j] - 'a' + 10);
				if ('A' <= argv[i][j] && argv[i][j] <= 'F') va = va * 16 + (argv[i][j] - 'A' + 10);
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
		} else {
			cprintf("Error address format: %s\n", argv[i]);
		}
	}
	return 0;
}

int 
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	return 0;
}
