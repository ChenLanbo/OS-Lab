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

// control block list
struct cb *cbl;
// receive frame area list
struct rfa *rfal;

static int cbl_head;
static int cbl_tail;
static int rfal_head;
static int rfal_tail;

void
cbl_init()
{
	int i;
	memset(cbl, 0, RING * sizeof(struct cb));
	for (i = 0; i < RING; i++){
		cbl[i].status = (1 << 15);
		cbl[i].link = PADDR(&cbl[(i+1) % RING]);
		cbl[i].array_addr = TBD_SIMPLE_ARRAY_ADDR;
		cbl[i].thresh = 0xe0;
	}
	cbl[0].command = ACTION_NOP | ACTION_FLAG_S;

	outl(nic_pcif.reg_base[1] + 0x4, PADDR(cbl));
	set_scb_command(SCB_CU_START);
	cbl_head = 1;
	cbl_tail = 1;
}

void
rfal_ini()
{
	int i;
	memset(rfal, 0, RING * sizeof(struct rfa));
	for (i = 0; i < RING; i++){
		;
	}
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
	cprintf("NIC_INIT DONE\n");
	return 0;
}

int
nic_insert_packet(void *buf, size_t size)
{
	if ((cbl_tail + 1) % RING == cbl_head){
		return -1;
	}
	memmove(cbl[cbl_tail].data, (char *)buf, size);
	cbl[cbl_tail].count = size;
	cbl[cbl_tail].status = (1 << 15);
	cbl[cbl_tail].command = ACTION_TRS | ACTION_FLAG_EL | ACTION_FLAG_S;
	cbl_tail = (cbl_tail + 1) % RING;
	return 0;
}


int 
nic_send_packet(void *buf, size_t size)
{
	uint16_t cur = get_scb_status();

	cprintf("nic_send_packet %d status %04x\n", cbl_tail, get_scb_status());
	nic_insert_packet(buf, size);
	// cprintf("status %x\n", get_scb_status());
	if (cur & SCB_STAT_CU_SUSPENDED){
		cprintf("SUSPENDED\n");
		set_scb_command(SCB_CU_RESUME);
	}
	if ((cur & 0xc0) == SCB_STAT_CU_IDLE){
		cprintf("IDLE\n");
		outl(nic_pcif.reg_base[1] + 0x4, PADDR(&cbl[(cbl_tail - 1 + RING) % RING]));
		set_scb_command(SCB_CU_START);
	}
	// cprintf("cbl_head stat: %x\n", cbl[cbl_head].status);
	while (cbl_head != cbl_tail){
		if ((cbl[cbl_head].status & (CU_STATUS_C | CU_STATUS_OK)) == (CU_STATUS_OK | CU_STATUS_C))
			cbl_head = (cbl_head + 1) % RING;
		else
			break;
	}
	return size;
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
