#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum
{
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_getenv_parent_id,
	SYS_env_destroy,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_trapframe,
	SYS_env_set_pgfault_upcall,
	// Lab 7
	SYS_env_get_curdir,
	SYS_env_set_curdir,
	SYS_yield,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_time_msec,
	// Lab 6 Network Driver
	SYS_net_send,
	SYS_net_recv,
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
