// LAB 6: Your driver code here
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/e100.h>
#include <kern/pmap.h>

#define DEBUG_E100 0

struct pci_func nic_pcif;

// control block list
// receive frame area list
// both allocated in kern/pmap.c
struct cb *cbl;
struct rfd *rfa;

static int cbl_head;
static int cbl_tail;
static int rfa_head;
static int rfa_tail;

// initialize control block list
void
cbl_init()
{
	int i;
	memset(cbl, 0, RING * sizeof(struct cb));
	for (i = 0; i < RING; i++){
		// here is important
		cbl[i].status = CU_STATUS_C;
		cbl[i].link = PADDR(&cbl[(i + 1) % RING]);
		cbl[i].array_addr = TBD_SIMPLE_ARRAY_ADDR;
		cbl[i].thresh = 0xe0;
	}
	cbl[0].command = COMMAND_NOP | COMMAND_FLAG_S;

	outl(nic_pcif.reg_base[1] + 0x4, PADDR(cbl));
	set_scb_command(SCB_CU_START);
	// LOG(DEBUG_E100, "CBL_INIT DONE\n");
	cbl_head = 0;
	cbl_tail = 1;
}

// initialize receive frame area
void
rfa_init()
{
	int i;
	memset(rfa, 0, RING * sizeof(struct rfd));
	for (i = 0; i < RING; i++){
		rfa[i].link = PADDR(&rfa[(i+1) % RING]);
		rfa[rfa_tail].padding = TBD_SIMPLE_ARRAY_ADDR;
		rfa[i].size = ETH_FRAME_SIZE;
	}
	// initial structure: the EL bit of the last rfd should be set
	// page 102 of intel 8255x
	rfa[RING - 1].command = RECEIVE_FLAG_EL;
	outl(nic_pcif.reg_base[1] + 0x4, PADDR(rfa));
	set_scb_command(SCB_RU_START);
	// LOG(DEBUG_E100, "RFA_INIT DONE\n");
	rfa_tail = 0;
}

int
nic_init(struct pci_func *pcif)
{
	int i;
	pci_func_enable(pcif);
	nic_pcif = *pcif;

	outl(pcif->reg_base[1] + 0x8, 0);
	for (i = 0; i < 8; i++){
		inb(0x84);
	}
	cbl_init();
	rfa_init();
	// LOG(DEBUG_E100, "NIC_INIT DONE\n");
	return 0;
}

int 
nic_send_packet(void *buf, size_t size)
{
	int ret;
	int cur = cbl_tail;
	int pre = (cbl_tail - 1  < 0 ? RING - 1 : cbl_tail - 1);
	uint16_t stat = get_scb_status();

	// if this cb's status is not CU_STATUS_C, return 0
	if ((cbl[cbl_tail].status & CU_STATUS_C) == 0){
		return 0;
	}
	size = MIN(size, ETH_FRAME_SIZE);
	memmove(cbl[cur].data, (char *)buf, size);

	cbl[cur].command = COMMAND_TRS | COMMAND_FLAG_S;
	cbl[cur].status = 0;
	cbl[cur].array_addr = TBD_SIMPLE_ARRAY_ADDR;
	cbl[cur].thresh = 0xe0;
	cbl[cur].count = size;
	cbl[pre].command &= ~COMMAND_FLAG_S;
	// ret = nic_cu_insert_packet(buf, size);
	cbl_tail = (cbl_tail + 1) % RING;
	if ((stat & SCB_STAT_CU_SUSPENDED)){
		set_scb_command(SCB_CU_RESUME);
	}
	return ret;
}

int
nic_recv_packet(void *buf, size_t size)
{
	int status = get_scb_status();
	int ret = size, i = rfa_tail, j;
	if ((rfa[i].status & RU_STATUS_C) == 0){
		return 0;
	}
	if (ret > (rfa[i].count & RU_COUNT_MASK)){
		ret = (rfa[i].count & RU_COUNT_MASK);
	}
	// LOG(DEBUG_E100, "INFO: NIC RECEIVES PACKET\n");
	memmove(buf, rfa[i].data, ret);
	// set to next rfd
	rfa[i].command = RECEIVE_FLAG_EL;
	rfa[i].status = 0;
	rfa[i].count = 0;
	j = (rfa_tail - 1 < 0 ? RING - 1 : rfa_tail - 1);
	rfa[j].command &= ~RECEIVE_FLAG_EL;

	rfa_tail = (rfa_tail + 1) % RING;
	
	if (status & SCB_STAT_RU_NORES){
		set_scb_command(SCB_RU_RESUME);
	}
	return ret;
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
