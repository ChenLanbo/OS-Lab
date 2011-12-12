#include <inc/lib.h>

int
umain(int argc, char **argv)
{
	int i;
	int r;

	if (argc == 1) {
		printf("usage: rm filename\n");
		return 1;
	}   
	for (i = 1; i < argc; ++i) {
		if ((r = remove(argv[i])) < 0) {
			printf("error in remove: %e\n", r); 
			return 1;
		}   
	}   

	return 0;
}

