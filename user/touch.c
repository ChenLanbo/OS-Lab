#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int i, r;
	for (i = 1; argv[i]; i++){
		r = open(argv[i], O_CREAT | O_EXCL);
		if (r < 0){
			cprintf("%s already exists\n", argv[i]);
		} else {
			close(r);
		}
	}
}
