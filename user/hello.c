// hello, world
#include <inc/lib.h>
#include <inc/elf.h>

void
umain(void)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", env->env_id);
}
