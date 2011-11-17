// LAB 6: Your driver code here
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/e100.h>
#include <kern/pmap.h>

struct pci_func nic_pcif;

// Ring size is 256
struct tcb *tcbs;
static int tcb_head;
static int tcb_tail;

void
dma_init()
{
	int i;
	for (i = 0; i < RING_SIZE; i++){
		tcbs[i].tcb_link = PADDR(&tcbs[(i+1)%RING_SIZE]);
	}
	tcb_head = tcb_tail = 0;
}

void
cbl_init(struct cb *cbl, int *phead, int *ptail)
{
	int i;
	memset(cbl, 0, RING_SIZE * sizeof(struct cb));
	*phead = 0;
	*ptail = 0;
	for (i = 0; i < RING_SIZE; i++){
		tcbs[i].tcb_link = PADDR(&tcbs[(i+1)%RING_SIZE]);
	}
}

uint32_t
get_tcb_head()
{
	return PADDR(&tcbs[tcb_head]);
}

// return 0 on success
// return -1 if the ring is full
// notify commant unit
int
insert_tcb(void *buf, size_t size)
{
	int pre;
	if ((tcb_tail + 1) % RING_SIZE == tcb_head){
		return -1;
	}

	memset(&tcbs[tcb_tail], 0, sizeof(struct tcb));
	// Copy data
	memmove(tcbs[tcb_tail].data, (char *)buf, size);
	tcbs[tcb_tail].tcb_cmd = COMMAND_TRS | TRS_EL;
	tcbs[tcb_tail].tcb_array_addr = TBD_SIMPLE_ARRAY_ADDR;
	tcbs[tcb_tail].tcb_byte_cnt = size;
	tcbs[tcb_tail].tcb_thrs = 1;
	if (tcb_tail != tcb_head){
		pre = (tcb_tail - 1 < 0 ? RING_SIZE - 1: tcb_tail - 1);
		if (tcbs[pre].tcb_cmd & TRS_EL){
			tcbs[pre].tcb_cmd ^= TRS_EL;
		}
	}
	cprintf("%s\n", (char *)buf);
	// cprintf("Done: insert into TCB RING -- size %d h %d t %d -- str: %s\n", size, tcb_head, tcb_tail, (char *)buf);
	tcb_tail = (tcb_tail + 1) % RING_SIZE;
	return 0;
}

uint16_t
get_scb_status()
{
	return inw(nic_pcif.reg_base[1]);
}

void
set_scb_status(uint16_t status)
{
	outw(nic_pcif.reg_base[1], status); 
}

uint16_t
get_scb_command()
{
	return inw(nic_pcif.reg_base[1] + 0x2);
}

void
set_scb_command(uint16_t command)
{
	outw(nic_pcif.reg_base[1] + 0x2, command);
}
