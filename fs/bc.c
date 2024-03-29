#include "fs.h"
#include <inc/assert.h>

#define DEBUG_BC 0
// LAB 5 Exercise 2 Challenge
static int pgfault_cnt = 0;
const static int threshold = PTSIZE;

// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (vpd[PDX(va)] & PTE_P) && (vpt[VPN(va)] & PTE_P);
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (vpt[VPN(va)] & PTE_D) != 0;
}

// LAB 5 Exercise 2 Challenge: block cache eviction
void
bc_reclaim(int blockno)
{	
	int x, y, num;
	void *addr1;
	pgfault_cnt++;
	if (pgfault_cnt < threshold){
		return ;
	}
	LOG(DEBUG_BC, "call bc_reclaim to reclaim pages\n");
	pgfault_cnt = 0;
	for (x = 0; x < BLKBITSIZE / 32; x++){
		// All blocks are free
		if ((bitmap[x] & 0xffffffff) == 0xffffffff){
			continue;
		}
		// Reclaim
		for (y = 0; y < 32; y++){
			if ((bitmap[x] & (1 << y)) == 0){
				num = x * 32 + y;
				// Debug info
				// skip boot sector, super block, bitmap block
				if (num > 2){
					addr1 = diskaddr(num);
					if (!(vpt[VPN(addr1)] & PTE_P)){
						continue;
					}
					if (vpt[VPN(addr1)] & PTE_A){
						LOG(DEBUG_BC, "Evict block %d\n", num);
						if (vpt[VPN(addr1)] & PTE_D){
							flush_block(addr1);
						}
						sys_page_unmap(0, addr1);
					}
				}
			}
		}
	}
}

// Fault any disk block that is read or written in to memory by
// loading it from disk.
// Hint: Use ide_read and BLKSECTS.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int i, r;

	void *addr_align = ROUNDDOWN(addr, BLKSIZE);
	uint32_t secno = ((uint32_t)addr_align - DISKMAP) / SECTSIZE;

	// Exercise 2 Challenge
	bc_reclaim(blockno);

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Allocate a page in the disk map region and read the
	// contents of the block from the disk into that page.
	//
	// LAB 5: Your code here
	if ((r = sys_page_alloc(sys_getenvid(), (void *)addr_align, PTE_U | PTE_P | PTE_W)) < 0){
		panic("sys_page_alloc error %e", r);
	}
	ide_read(secno, addr_align, BLKSECTS);

	// Sanity check the block number. (exercise for the reader:
	// why do we do this *after* reading the block in?)
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Check that the block we read was allocated.
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
// Hint: Use va_is_mapped, va_is_dirty, and ide_write.
// Hint: Use the PTE_USER constant when calling sys_page_map.
// Hint: Don't forget to round addr down.
void
flush_block(void *addr)
{
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	void *addr_align = ROUNDDOWN(addr, BLKSIZE);
	int secno = ((uint32_t)addr_align - DISKMAP) / SECTSIZE;
	envid_t id = sys_getenvid();

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);

	// LAB 5: Your code here.
	// Check mapped
	if (!va_is_mapped(addr)){
		return ;
	}
	// Check dirty
	if (!va_is_dirty(addr)){
		return ;
	}
	ide_write(secno, addr_align, BLKSECTS);
	sys_page_map(id, addr_align, id, addr_align, PTE_P | PTE_U | PTE_W);
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it 
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	set_pgfault_handler(bc_pgfault);
	check_bc();
}

