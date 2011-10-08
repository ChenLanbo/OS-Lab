// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(void)
{
	asm volatile("int $3");
	asm volatile("movl $8, %eax");
	asm volatile("int $3");
}

