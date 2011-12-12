#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	char pathname[128];
	memset(pathname, 0, sizeof(pathname));
	sys_env_get_curdir(0, pathname);
	cprintf("%s\n", pathname);
}
