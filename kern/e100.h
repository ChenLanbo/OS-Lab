#ifndef JOS_KERN_E100_H
#define JOS_KERN_E100_H
#include <inc/types.h>
#include <kern/pci.h>

// Command Block
struct cb {
	volatile uint16_t status;
	uint16_t cmd;
	uint32_t link;
	char data[1592];
};

// Tansmit command block
struct tcb {
	volatile uint16_t tcb_status;
	uint16_t tcb_cmd;
	uint32_t tcb_link;
	uint32_t tcb_array_addr;
	uint16_t tcb_byte_cnt;
	uint16_t tcb_thrs;
	char data[1584];
};

extern struct tcb *tcbs;

#define RING_SIZE 256
#define TBD_SIMPLE_ARRAY_ADDR 0xffffffff


//
// System control block command
//
// No operation
#define COMMAND_NOP 0x0
// Individual address setup
#define COMMAND_IAS 0x1
// Configure
#define COMMAND_CFG 0x2
// Multicast setup
#define COMMAND_MTS 0x3
// Transmit
#define COMMAND_TRS 0x4
// Load microcode
#define COMMAND_LMC 0x5
// Dump
#define COMMAND_DMP 0x6
// Diagnose
#define COMMAND_DGN 0x7

// Transmit flag
// EL: last one in CBL
#define TRS_EL 0x8000
// S : suspend
#define TRS_S  0x4000
#define TRS_I  0x2000

#define IS_CU_IDEL(s) ((((s) >> 6) & 0x3) == 0)
#define IS_CU_SUSPENDED(s) ((((s) >> 6) & 0x3) == 1)
#define IS_CU_LPQ(s) ((((s) >> 6) & 0x3) == 2)
#define IS_CU_HQP(s) ((((s) >> 6) & 0x3) == 3)

#define SET_CUC_START(c) ((c ^ (c & 0xf0)) | 0x30)
#define SET_CUC_BASE(c) ((c ^ (c & 0xf0)) | 0x60)

extern struct pci_func nic_pcif;

void dma_init();
void cbl_init(struct cb *cbl, int *phead, int *ptail);
uint32_t get_tcb_head();
int insert_tcb(void *buf, size_t size);

uint16_t get_scb_status();
void set_scb_status(uint16_t status);
uint16_t get_scb_command();
void set_scb_command(uint16_t command);

#endif	// JOS_KERN_E100_H
