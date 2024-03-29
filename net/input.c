#include "ns.h"
#include <inc/lib.h>
#include <inc/assert.h>

#define DEBUG_INPUT 0
extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";
	int r;
	struct jif_pkt *packet = (struct jif_pkt *)REQVA;
	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	while (1){
		if ((r = sys_page_alloc(0, packet, PTE_U | PTE_P | PTE_W)) < 0){
			panic("sys_page_alloc error");
		}
		r = sys_net_recv(packet->jp_data, 1518);
		packet->jp_len = r;
		LOG(DEBUG_INPUT, "ns_input gets %d bytes\n", r);
		if (packet->jp_len == 0){
			sys_yield();
		} else {
			ipc_send(ns_envid, NSREQ_INPUT, packet, PTE_P | PTE_U | PTE_W);
		}
		sys_page_unmap(0, packet);
	}
}
