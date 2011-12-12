#include <inc/lib.h>

int
umain(int argc, char **argv)
{
	int i, r;
	if (argc == 1){
		cprintf("usage: mkdir directory ...\n");
		return -1;
	}
	for (i = 1; argv[i]; i++){
		if ((r = open(argv[i], O_CREAT | O_EXCL | O_MKDIR)) < 0){
			cprintf("%s already exists\n");
			return -1;
		} else {
			close(r);
		}
	}
	return 0;
}
