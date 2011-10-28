/* My own implementation of interprocess communication */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/stdio.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/monitor.h>

#include <kern/myipc.h>

struct Myipc *myipcs;
static struct Myipc_list myipc_free_list;
static struct Myipc_list myipc_queue_list;
// static struct Myipc_queue queue;

void
myipc_init(void)
{
	int i;
	LIST_INIT(&myipc_free_list);
	LIST_INIT(&myipc_queue_list);
	for (i = NMYIPC - 1; i >= 0; i--){
		myipcs[i].ipc_to = 0;
		LIST_INSERT_HEAD(&myipc_free_list, &myipcs[i], ipc_link);
	}
}

// Allocate a new message
int
myipc_alloc(struct Myipc **ipc_store)
{
	if (LIST_EMPTY(&myipc_free_list)){
		return -E_MYIPC;
	} else {
		*ipc_store = LIST_FIRST(&myipc_free_list);
		LIST_REMOVE(LIST_FIRST(&myipc_free_list), ipc_link);
		return 0;
	}
}

// Free poped message
void
myipc_free(struct Myipc *ipc)
{
	LIST_INSERT_HEAD(&myipc_free_list, ipc, ipc_link);
}

// Push a new message at the end of the queue
int
myipc_queue_push(envid_t to, envid_t from, uint32_t value, void *va, int perm)
{
	struct Myipc *pp = NULL;
	myipc_queue_clean();
	if (myipc_alloc(&pp) < 0){
		return -E_MYIPC;
	}
	pp->ipc_to = to;
	pp->ipc_from = from;
	pp->ipc_value = value;
	pp->ipc_va = va;
	pp->ipc_perm = perm;

	LIST_INSERT_HEAD(&myipc_queue_list, pp, ipc_link);
	return 0;
}

// Pop a message at the front of the queue
struct Myipc *
myipc_queue_pop(envid_t to)
{
	struct Myipc *var, *ret = NULL;
	myipc_queue_clean();
	LIST_FOREACH(var, &myipc_queue_list, ipc_link){
		if (var->ipc_to == to){
			ret = var;
		}
	}
	if (ret != NULL){
		LIST_REMOVE(ret, ipc_link);
		LIST_INSERT_HEAD(&myipc_free_list, ret, ipc_link);
	}
	// Debug info
	// cprintf("Queue:");
	// LIST_FOREACH(var, &myipc_queue_list, ipc_link){
	// 	cprintf(" %x", var->ipc_from);
	// }
	// cprintf("\n");
	return ret;
}

// Clean invalid message
void
myipc_queue_clean()
{
	struct Myipc *var;
	struct Env *env;
	int mark = 1;
	// cprintf("Clean\n");
	while (mark){
		mark = 0;
		LIST_FOREACH(var, &myipc_queue_list, ipc_link){
			// target environment is destroyed,
			// delete this message from the queue
			if (envid2env(var->ipc_to, &env, 0) < 0){
				if (envid2env(var->ipc_from, &env, 0) < 0){
				} else {
					// src environment
					// cprintf("*** %x\n", env->env_id);
					env->env_status = ENV_RUNNABLE;
					env->env_tf.tf_regs.reg_eax = -E_INVAL;
				}
				mark = 1;
				LIST_REMOVE(var, ipc_link);
				LIST_INSERT_HEAD(&myipc_free_list, var, ipc_link);
				break;
			}
		}
	}
}
