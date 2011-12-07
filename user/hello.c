// hello, world
#include <inc/lib.h>
#include <inc/elf.h>

int check(int fd, struct Elf *elf, char *elf_buf){
	int r;
	if ((r = read(fd, elf_buf, sizeof(elf_buf))) != sizeof(elf_buf)
			|| elf->e_magic != ELF_MAGIC) {
		close(fd);
		cprintf("elf magic %08x want %08x --- %d bytes\n", elf->e_magic, ELF_MAGIC, r);
		return -E_NOT_EXEC;
	}
	cprintf("GOOD CHECK\n");
	return 0;
}

void
umain(void)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", env->env_id);
	int r, a, b, pipe_child = 0;
	const char *arg = NULL;
	int p[2];
	// pipe(p);
	p[0] = p[1] = -1;
	if ((r = fork()) < 0){
		exit();
	}
	if (r == 0){
		cprintf("CHILD\n");
		if (p[0] != 0){
			dup(p[0], 0);
			close(p[0]);
		}
		close(p[1]);
		if ((r = spawn("cat", &arg)) < 0){
			cprintf("CHILD ERROR\n");
		} else {
			cprintf("CHILD SPAWN GOOD %x\n", r);
		}
		if (r >= 0){
			wait(r);
		}
	} else {
		cprintf("PARENT\n");
		pipe_child = r;
		if (p[1] != 1){
			dup(p[1], 1);
			close(p[1]);
		}
		close(p[0]);
		if ((r = spawn("ls", &arg)) < 0){
			cprintf("PARENT ERROR\n");
		} else {
			cprintf("PARENT SPAWN GOOD %x\n", r);
		}
		if (r >= 0){
			wait(r);
		}
		wait(pipe_child);
	}

	/*if ((r = fork()) == 0){
		if ((b = open("hello", O_RDONLY)) < 0)
			exit();
		char b1[512];
		struct Elf *e1 = (struct Elf *)b1;
		if (check(b, e1, b1) != 0){
			cprintf("ERROR CHILD\n");
		}
		exit();
	}
	if ((a = open("num", O_RDONLY)) < 0)
		exit();
	char b2[512];
	struct Elf *e2 = (struct Elf *)b2;
	if (check(a, e2, b2) != 0){
		cprintf("ERROR PARENT\n");
	}
	exit();*/
}
