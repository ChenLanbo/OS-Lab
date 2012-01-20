#include "ns.h"
#include <inc/lib.h>
#include <inc/assert.h>

#define DEBUG_OUTPUT 0
extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";
	envid_t from;
	int r, perm;
	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	while (1){
		r = ipc_recv(&from, &nsipcbuf, &perm);
		if (r < 0){
			panic("ipc_recv error");
		}
		if (from != ns_envid){
			panic("bad environment %x", from);
		}
		if (r == NSREQ_OUTPUT){
			LOG(DEBUG_OUTPUT, "net output send out packet\n");
			sys_net_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
		}
	}
}

