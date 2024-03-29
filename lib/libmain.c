// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, vpd, and vpt.
#include <inc/assert.h>
#include <inc/lib.h>
#define DEBUG_LIBMAIN 0
extern void umain(int argc, char **argv);

volatile struct Env *env;
char *binaryname = "(PROGRAM NAME UNKNOWN)";

void
libmain(int argc, char **argv)
{
	// set env to point at our env structure in envs[].
	// LAB 3: Your code here.
	envid_t pid;

	env = 0;
	pid = sys_getenvid();
	env = (struct Env *)envs;
	env = env + ENVX(pid);
	// Debug info
	// cprintf("pid %u\n", pid);
	// cprintf("off %u\n", ENVX(pid));
	LOG(DEBUG_LIBMAIN, "In libmain, about to start user program %x\n", pid);

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

