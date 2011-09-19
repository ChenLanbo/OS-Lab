#ifndef JOS_KERN_MEMUTIL_H
#define JOS_KERN_MEMUTIL_H

struct Trapframe;
int mon_showmappings(int argc, char **argv, struct Trapframe *tf);
int mon_memdump(int argc, char **argv, struct Trapframe *tf);
#endif
