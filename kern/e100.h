#ifndef JOS_KERN_E100_H
#define JOS_KERN_E100_H
#include <inc/types.h>
#include <kern/pci.h>

#define ETH_FRAME_SIZE 1518

#define SCB_CU_NOP (0<<4)
#define SCB_CU_START (1<<4)
#define SCB_CU_RESUME (2<<4)
#define SCB_CU_LOADBASE (6<<4)

#define SCB_STAT_CU_IDLE (0 << 6)
#define SCB_STAT_CU_SUSPENDED (1 << 6)
#define SCB_STAT_CU_LPQ (2 << 6)
#define SCB_STAT_CU_HQP (3 << 6)

/*
 * Action Command
 */

// No operation
#define ACTION_NOP 0x0
// Individual address setup
#define ACTION_IAS 0x1
// Configure
#define ACTION_CFG 0x2
// Multicast setup
#define ACTION_MTS 0x3
// Transmit
#define ACTION_TRS 0x4
// Load microcode
#define ACTION_LMC 0x5
// Dump
#define ACTION_DMP 0x6
// Diagnose
#define ACTION_DGN 0x7

#define ACTION_FLAG_EL 0x8000
#define ACTION_FLAG_S  0x4000
#define ACTION_FLAG_I  0x2000

#define CU_STATUS_C (1 << 15)
#define CU_STATUS_OK (1 << 13)
#define CU_STATUS_U (1 << 12)

#define RING 128
#define TBD_SIMPLE_ARRAY_ADDR 0xffffffff


// Command block

struct cb {
	volatile uint16_t status;
	uint16_t command;
	uint32_t link;
	uint32_t array_addr;
	uint16_t count;
	uint16_t thresh;
	char data[ETH_FRAME_SIZE];
}__attribute__((aligned(4), packed));

struct rfa {
	volatile uint16_t status;
	uint16_t command;
	uint32_t link;
	uint32_t padding;
	uint16_t count;
	uint16_t size;
	char data[ETH_FRAME_SIZE];
}__attribute__((aligned(4), packed));

extern struct cb *cbl;
extern struct rfa *rfal;

int nic_init(struct pci_func *pcif);
int nic_send_packet(void *buf, size_t size);

uint16_t get_scb_status();
uint16_t get_scb_command();
void set_scb_status(uint16_t status);
void set_scb_command(uint16_t command);

#endif	// JOS_KERN_E100_H
