#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	// Search through 'envs' for a runnable environment,
	// in circular fashion starting after the previously running env,
	// and switch to the first such environment found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.

	// LAB 4: Your code here.

	envid_t curpid, nxtpid;

	/*if (curenv == NULL){
		cprintf("First run %d\n", 1);
		env_run(&envs[1]);
		return;
	}*/
	if (curenv == NULL){
		curpid = 0;
	} else {
		curpid = curenv - envs;
	}

	cprintf("Curenv %d\n", curpid);
	for (nxtpid = curpid + 1; nxtpid < NENV; nxtpid++){
		if (envs[nxtpid].env_id != 0 && envs[nxtpid].env_status == ENV_RUNNABLE){
			break;
		}
	}

	// cprintf("Next candidate %d\n", nxtpid);
	if (nxtpid == NENV){
		for (nxtpid = 1; nxtpid != curpid && nxtpid < NENV; nxtpid++){
			if (envs[nxtpid].env_id != 0 && envs[nxtpid].env_status == ENV_RUNNABLE){
				break;
			}
		}

		if (nxtpid == NENV){
			nxtpid = 0;
		}
	}

	if (nxtpid != 0){
		cprintf("Next run %d, yield %d\n", envs[nxtpid].env_id, envs[curpid].env_id);
		env_run(&envs[nxtpid]);
		return ;
	}

	// Run the special idle environment when nothing else is runnable.
	if (envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else {
		cprintf("Destroyed all environments - nothing more to do!\n");
		while (1)
			monitor(NULL);
	}
}
