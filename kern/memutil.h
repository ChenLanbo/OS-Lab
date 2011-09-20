#ifndef JOS_KERN_MEMUTIL_H
#define JOS_KERN_MEMUTIL_H
// lab2 challenges
// general JOS kernel monitor commands for memory management
// showmappings: show the physical page mappings of the corresponding virtual addresses
// memdump: show the contents of a range of memory

struct Trapframe;

int mon_showmappings(int argc, char **argv, struct Trapframe *tf);
int mon_memdump(int argc, char **argv, struct Trapframe *tf);

// alloc_page: allocate a page of 4KB
// page_status: show physical page status
// free_page: remove allocated pages
int mon_alloc_page(int argc, char **argv, struct Trapframe *tf);
int mon_page_status(int argc, char **argv, struct Trapframe *tf);
int mon_free_page(int argc, char **argv, struct Trapframe *tf);
#endif
