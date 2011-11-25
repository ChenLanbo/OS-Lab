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
	// struct jif_pkt *packet;

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	cprintf("ns_output\n");
	while (1){
		// packet = (struct jif_pkt *)REQVA;
		r = ipc_recv(&from, &nsipcbuf, &perm);
		if (from != ns_envid){
			panic("bad environment %x", from);
		}
		if (r == NSREQ_OUTPUT){
			cprintf("****************\n");
			sys_net_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
			// sys_net_send(packet->jp_data, packet->jp_len);
		}
	}
}

