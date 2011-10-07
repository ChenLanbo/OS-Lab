// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(void)
{
	int a = 10;
	asm volatile("int $3");
	a++;
	a++;
	asm volatile("movl $8, %eax");
	asm volatile("int $3");
}

