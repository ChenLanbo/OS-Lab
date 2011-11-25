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
struct rfd *rfa;

static int cbl_head;
static int cbl_tail;
static int rfa_head;
static int rfa_tail;

void
cbl_init()
{
	int i;
	memset(cbl, 0, RING * sizeof(struct cb));
	for (i = 0; i < RING; i++){
		cbl[i].status = CU_STATUS_C;
		cbl[i].link = PADDR(&cbl[(i + 1) % RING]);
		cbl[i].array_addr = TBD_SIMPLE_ARRAY_ADDR;
		cbl[i].thresh = 0xe0;
	}
	cbl[0].command = COMMAND_NOP | COMMAND_FLAG_S;

	outl(nic_pcif.reg_base[1] + 0x4, PADDR(cbl));
	set_scb_command(SCB_CU_START);
	cprintf("CBL_INIT DONE\n");
	cbl_head = 0;
	cbl_tail = 1;
}

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
	// Initial structure: the EL bit of the last rfd should be set
	rfa[RING - 1].command = RECEIVE_FLAG_EL;
	outl(nic_pcif.reg_base[1] + 0x4, PADDR(rfa));
	set_scb_command(SCB_RU_START);
	cprintf("RFA_INIT DONE\n");
	for (i = 0; i < RING; i++){
		cprintf("RFA_INIT: %d %x\n", i, rfa[i].status);
	}
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
	cprintf("NIC_INIT DONE\n");
	return 0;
}

int
nic_cu_insert_packet(void *buf, size_t size)
{
	int cur = cbl_tail, pre = (cbl_tail - 1  < 0 ? RING - 1 : cbl_tail - 1);
	while (cbl_head != cbl_tail){
		if ((cbl[cbl_head].status & CU_STATUS_C) == 0){
			break;
		}
		cbl_head++;
	}
	size = MIN(size, ETH_FRAME_SIZE);
	memmove(cbl[cur].data, (char *)buf, size);

	cbl[cur].command = COMMAND_TRS | COMMAND_FLAG_S;
	cbl[cur].status = 0;
	cbl[cur].array_addr = TBD_SIMPLE_ARRAY_ADDR;
	cbl[cur].thresh = 0xe0;
	cbl[cur].count = size;
	cbl[pre].command &= ~COMMAND_FLAG_S;

	/*int cur = cbl_tail, pre = (cbl_tail - 1  < 0 ? RING - 1 : cbl_tail - 1);
	int i;
	// for (i = 0; i < RING; i++){ if ((cbl[i].status & CU_STATUS_C)){ cprintf("I %d *****************\n", i); } }
	if ((cbl[pre].status & CU_STATUS_C) == 0){
		return 0;
	}
	size = MIN(size, ETH_FRAME_SIZE);
	memmove(cbl[cur].data, (char *)buf, size);

	cbl[cur].command = COMMAND_TRS | COMMAND_FLAG_S;
	cbl[cur].status = 0;
	cbl[cur].array_addr = TBD_SIMPLE_ARRAY_ADDR;
	cbl[cur].thresh = 0xe0;
	cbl[cur].count = size;
	cbl[pre].command &= ~COMMAND_FLAG_S;*/
	return size;
}

int 
nic_send_packet(void *buf, size_t size)
{
	int ret, nsend = 0, pre;
	uint16_t stat = get_scb_status();

	// cprintf("nic_send_packet %d status %04x\n", cbl_tail, stat);
	while (size != 0){
		if (!(cbl[cbl_tail].status & CU_STATUS_C)){
			break;
		}
		ret = nic_cu_insert_packet(buf, size);
		// cprintf("RET: %d %d\n", ret, size);
		if (ret == 0)
			break;
		nsend += ret;
		size -= ret;
		cbl_tail = (cbl_tail + 1) % RING;
	}

	if (nsend != 0 && (stat & SCB_STAT_CU_SUSPENDED)){
		// cprintf("SUSPENDED\n");
		set_scb_command(SCB_CU_RESUME);
	}
	return nsend;
	/*nic_cu_insert_packet(buf, size);
	// cprintf("status %x\n", get_scb_status());
	if (cur & SCB_STAT_CU_SUSPENDED){
		// cprintf("SUSPENDED\n");
		set_scb_command(SCB_CU_RESUME);
	}
	if ((cur & 0xc0) == SCB_STAT_CU_IDLE){
		// cprintf("IDLE\n");
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
	return size;*/
}

int
nic_ru_get_packet(void *buf, size_t size)
{
	int ret = size, i, j;
	// cprintf("~~~~~~~~~~~~~~ nic_ru_insert_packet\n");
	/*for (j = 0; j < RING; j++){
		cprintf("RING %d %x %d\n", j, rfa[j].status, rfa[j].count & RU_COUNT_MASK);
		if (rfa[j].status & RU_STATUS_C){
			cprintf("*** ASDFASDF *** %d t%d %d\n", j, rfa_tail, rfa[j].count & RU_COUNT_MASK);
			break;
		}
	}*/
	i = rfa_tail;
	if ((rfa[i].status & RU_STATUS_C) == 0){
		// cprintf("****** not complete\n");
		return 0;
	}

	if (ret > (rfa[i].count & RU_COUNT_MASK)){
		ret = (rfa[i].count & RU_COUNT_MASK);
	}

	memmove(buf, rfa[i].data, ret);
	j = rfa_tail - 1;
	if (j < 0) j = RING - 1;
	rfa[rfa_tail].command = RECEIVE_FLAG_EL;
	// rfa[rfa_tail].status = 0;
	rfa[rfa_tail].count = 0;
	rfa[rfa_tail].size = ETH_FRAME_SIZE;
	rfa[j].command &= ~RECEIVE_FLAG_EL;
	return ret;
}

int
nic_recv_packet(void *buf, size_t size)
{
	int status = get_scb_status();
	int ret, pre;
	if ((ret = nic_ru_get_packet(buf, size)) == 0){
		return 0;
	}
	// cprintf("$$$$$$$$$ nic_recv_packet %d\n", ret);
	rfa_tail = (rfa_tail + 1) % RING;
	
	if (status & SCB_STAT_RU_NORES){
		// cprintf("!!!!!!!!!! nic_recv_packet NO RESOURCES\n");
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
