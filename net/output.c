#include "ns.h"
#include <inc/lib.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";
	envid_t from;
	int r;
	int perm;
	struct jif_pkt *packet;

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	cprintf("ns_output\n");
	while (1){
		packet = (struct jif_pkt *)REQVA;
		r = ipc_recv(&from, (void *)REQVA, &perm);
		if (r == NSREQ_OUTPUT){
			printf("env %x get from env %x ret %d len %d str %s\n", sys_getenvid(), from, r, packet->jp_len, packet->jp_data);
			sys_net_send(packet->jp_data, packet->jp_len);
		}
	}
}

