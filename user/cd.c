#include <inc/lib.h>

int
umain(int argc, char **argv)
{
	int l1, l2;
	envid_t ppid, pppid;
	struct Stat s;
	char real_path[MAXNAMELEN];
	// cprintf("ppid %x\n", sys_getenv_parent_id());
	// cprintf("pid %x\n", sys_getenvid());
	ppid = sys_getenv_parent_id(0);
	pppid = sys_getenv_parent_id(ppid);

	if (argc == 1){
		sys_env_set_curdir(pppid, "/");
		return 0;
	}
	if (stat(argv[1], &s) < 0){
		cprintf("%s does not exists\n", argv[1]);
		return -1;
	}
	if (!s.st_isdir){
		cprintf("%s is a regular file\n", argv[1]);
		return -1;
	}

	memset(real_path, 0, sizeof(real_path));
	if (argv[1][0] == '/'){
		// real path
		strcpy(real_path, argv[1]);
	} else {
		sys_env_get_curdir(pppid, real_path);
		l1 = strlen(real_path);
		l2 = strlen(argv[1]);
		if (l1 + l2 >= MAXNAMELEN){
			cprintf("Exceeds the maximum path jos can support\n");
			return -1;
		} else {
			if (real_path[l1-1] == '/'){
				strcpy(real_path + l1, argv[1]);
			} else {
				real_path[l1] = '/';
				strcpy(real_path + l1 + 1, argv[1]);
			}
		}
	}
	// cprintf("REAL_PATH: %s\n", real_path);
	sys_env_set_curdir(pppid, real_path);
	return 0;
}
