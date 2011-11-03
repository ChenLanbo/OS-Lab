#ifndef JOS_KERN_DEBUGGER_H
#define JOS_KERN_DEBUGGER_H

// lab3b challenge
// general jos kernel monitor debugger commands

struct Trapframe;

// continue to run the program until breakpoints
int mon_continue(int argc, char **argv, struct Trapframe *tf);
// single-step program
int mon_si(int argc, char **argv, struct Trapframe *tf);

#endif
