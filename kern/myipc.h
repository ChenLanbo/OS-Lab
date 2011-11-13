#ifndef JOS_KERN_MYIPC_H
#define JOS_KERN_MYIPC_H
#include <inc/types.h>
#include <inc/queue.h>
#include <inc/trap.h>
#include <inc/env.h>
#include <inc/memlayout.h>

#define NMYIPC (1 << 10)
#define E_MYIPC 19881126

// Message structure
struct Myipc {
	LIST_ENTRY(Myipc) ipc_link;
	envid_t ipc_to;
	envid_t ipc_from;
	uint32_t ipc_value;
	void * ipc_va; // virtual address to map
	int ipc_perm; // permission
};

LIST_HEAD(Myipc_list, Myipc);
extern struct Myipc *myipcs;

// Functions
// Initialization
void myipc_init(void);
// Push a message into the message queue
int myipc_queue_push(envid_t to, envid_t from, uint32_t value, void *va, int perm);
// Pop a message from the message queue
struct Myipc * myipc_queue_pop(envid_t to);
// Clean message queue for invalid messages
void myipc_queue_clean();

#endif
