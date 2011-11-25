#ifndef JOS_KERN_E100_H
#define JOS_KERN_E100_H
#include <inc/types.h>
#include <kern/pci.h>

#define ETH_FRAME_SIZE 1518

#define SCB_CU_NOP (0<<4)
#define SCB_CU_START (1<<4)
#define SCB_CU_RESUME (2<<4)
#define SCB_CU_LOADBASE (6<<4)

#define SCB_RU_NOP 0
#define SCB_RU_START 1
#define SCB_RU_RESUME 2
#define SCB_RU_LOADBASE 6

#define SCB_STAT_CU_IDLE (0 << 6)
#define SCB_STAT_CU_SUSPENDED (1 << 6)
#define SCB_STAT_CU_LPQ (2 << 6)
#define SCB_STAT_CU_HQP (3 << 6)

#define SCB_STAT_RU_IDLE (0 << 2)
#define SCB_STAT_RU_SUSPENDED (1 << 2)
#define SCB_STAT_RU_NORES (2 << 2)
#define SCB_STAT_RU_READY (4 << 2)
#define SCB_STAT_RU_MASK (SCB_STAT_RU_IDLE | SCB_STAT_RU_SUSPENDED | SCB_STAT_RU_NORES | SCB_STAT_RU_READY)

/*
 * Action Command
 */

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

/*
 * Receive Command

// No operation
#define RECEIVE_NOP 0x0
#define RECEIVE_START 0x1
#define RECEIVE_RESUME 0x2
#define RECEIVE_DMA_REDIRECT 0x3
#define RECEIVE_ABORT 0x4
#define RECEIVE_HDS 0x5
#define RECEIVE_L
 */

#define COMMAND_FLAG_EL 0x8000
#define COMMAND_FLAG_S  0x4000
#define COMMAND_FLAG_I  0x2000

#define RECEIVE_FLAG_EL 0x8000
#define RECEIVE_FLAG_S  0x4000
#define RECEIVE_FLAG_H  0x0010
#define RECEIVE_FLAG_NONSF 0x0008

#define CU_STATUS_C (1 << 15)
#define CU_STATUS_OK (1 << 13)
#define CU_STATUS_U (1 << 12)

#define RU_STATUS_C (1 << 15)
#define RU_STATUS_OK (1 << 13)
#define RU_COUNT_MASK ((1 << 14) - 1)

#define RING 32
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

struct rfd {
	volatile uint16_t status;
	uint16_t command;
	uint32_t link;
	uint32_t padding;
	uint16_t count;
	uint16_t size;
	char data[ETH_FRAME_SIZE];
}__attribute__((aligned(4), packed));

extern struct cb *cbl;
extern struct rfd *rfa;

int nic_init(struct pci_func *pcif);
int nic_send_packet(void *buf, size_t size);
int nic_recv_packet(void *buf, size_t size);

uint16_t get_scb_status();
uint16_t get_scb_command();
void set_scb_status(uint16_t status);
void set_scb_command(uint16_t command);

#endif	// JOS_KERN_E100_H
