#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/mmu.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/kdebug.h>

// opcode names
static const char * const opcnames[] = {
	"00 ADD",
	"01 ADD",
	"02 ADD",
	"03 ADD",
	"04 ADD",
	"05 ADD",
	"06 PUSH ES",
	"07 POP  ES",
	"08 OR",
	"09 OR",
	"0A OR",
	"0B OR",
	"0C OR",
	"0D OR",
	"0E PUSH CS",
	"0F TWO BYTE INSTRUCTION",
	"10 ADC",
	"11 ADC",
	"12 ADC",
	"13 ADC",
	"14 ADC",
	"15 ADC",
	"16 PUSH SS",
	"17 POP  SS",
	"18 SBB ",
	"19 SBB ",
	"1A SBB ",
	"1B SBB ",
	"1C SBB ",
	"1D SBB ",
	"1E PUSH DS",
	"1F POP  DS",
	"20 AND",
	"21 AND",
	"22 AND",
	"23 AND",
	"24 AND",
	"25 AND",
	"26 ES ",
	"27 DAA",
	"28 SUB",
	"29 SUB",
	"2A SUB",
	"2B SUB",
	"2C SUB",
	"2D SUB",
	"2E CS",
	"2F DAS",
	"30 XOR",
	"31 XOR",
	"32 XOR",
	"33 XOR",
	"34 XOR",
	"35 XOR",
	"36 SS",
	"37 AAA ",
	"38 CMP ",
	"39 CMP ",
	"3A CMP ",
	"3B CMP ",
	"3C CMP ",
	"3D CMP ",
	"3E DS",
	"3F AAS",
	"40 INC",
	"41 INC",
	"42 INC",
	"43 INC",
	"44 INC",
	"45 INC",
	"46 INC",
	"47 INC",
	"48 DEC",
	"49 DEC",
	"4A DEC",
	"4B DEC",
	"4C DEC",
	"4D DEC",
	"4E DEC",
	"4F DEC",
	"50 PUSH",
	"51 PUSH",
	"52 PUSH",
	"53 PUSH",
	"54 PUSH",
	"55 PUSH",
	"56 PUSH",
	"57 PUSH",
	"58 POP",
	"59 POP",
	"5A POP",
	"5B POP",
	"5C POP",
	"5D POP",
	"5E POP",
	"5F POP",
	"60 PUSHAD",
	"61 POPAD",
	"62 BOUND",
	"63 ARPL",
	"64 FS",
	"64 GS",
	"66 NONE",
	"67 NONE",
	"68 PUSH",
	"69 IMUL",
	"6A PUSH",
	"6B IMUL",
	"6C INS",
	"6D INS",
	"6E OUTS",
	"6F OUTS",
	"70 JO",
	"71 JNO",
	"72 JB",
	"73 JNB",
	"74 JZ",
	"75 JNZ",
	"76 JBE",
	"77 JNBE",
	"78 JS",
	"79 JNS",
	"7A JP",
	"7B JNP",
	"7C JL",
	"7D JNL",
	"7E JLE",
	"7F JNLE",
	"80 ADD r/m8",
	"81 ADD r/m1",
	"82 ADD r/m8",
	"83 ADD r/m1",
	"84 TEST ",
	"85 TEST ",
	"86 XCHG ",
	"87 XCHG ",
	"88 MOV r/m8",
	"89 MOV r/m1",
	"8A MOV r8 ",
	"8B MOV r16/",
	"8C MOV m16 MOV r16/32 Sreg  ",
	"8D LEA r16/",
	"8E MOV Sreg",
	"8F POP r/m1",
	"90 XCHG ",
	"91 XCHG ",
	"92 XCHG ",
	"93 XCHG ",
	"94 XCHG ",
	"95 XCHG ",
	"96 XCHG ",
	"97 XCHG ",
	"98 CWDE EAX AX",
	"99 CDQ EDX ",
	"9A CALLF ",
	"9B FWAIT",
	"9C PUSHFD ",
	"9D POPFD ",
	"9E SAHF ",
	"9F LAHF ",
	"A0 MOV",
	"A1 MOV",
	"A2 MOV",
	"A3 MOV",
	"A4 MOVSB m8 m8",
	"A5 MOVSD m32 m32",
	"A6 CMPSB m8 m8",
	"A7 CMPSD m32 m32",
	"A8 TEST",
	"A9 TEST",
	"AA STOSB",
	"AB STOSD",
	"AC LODSB",
	"AD LODSD",
	"AE SCAS",
	"AF SCASD",
	"B0 MOV",
	"B1 MOV",
	"B2 MOV",
	"B3 MOV",
	"B4 MOV",
	"B5 MOV",
	"B6 MOV",
	"B7 MOV",
	"B8 MOV",
	"B9 MOV",
	"BA MOV",
	"BB MOV",
	"BC MOV",
	"BD MOV",
	"BE MOV",
	"BF MOV",
	"C0 SHR ",
	"C1 SHL",
	"C2 RETN ",
	"C3 RETN ",
	"C4 LES ES ",
	"C5 LDS DS ",
	"C6 MOV",
	"C7 MOV",
	"C8 ENTER ",
	"C9 LEAVE ",
	"CA RETF ",
	"CB RETF ",
	"CC INT 3",
	"CD INT imm8",
	"CE INTO ",
	"CF IRET ",
	"D0 ROL",
	"D1 ROL",
	"D2 ROR",
	"D3 RCL",
	"D4 AMX AL ",
	"D5 ADX AL ",
	"D6 SALC",
	"D7 XLAT",
	"D8 FADD",
	"D9 FLD",
	"DA FIADD ",
	"DB FILD ",
	"DC FADD ",
	"DD FUCOMP ",
	"DE FIMUL ",
	"DF FXCH7 ",
	"E0 LOOPNZ",
	"E1 LOOPZ",
	"E2 LOOP ",
	"E3 JCXZ",
	"E4 IN AL",
	"E5 IN EAX ",
	"E6 OUT imm8",
	"E7 OUT imm8",
	"E8 CALL ",
	"E9 JMP rel1",
	"EA JMPF ",
	"EB JMP rel8",
	"EC IN AL ",
	"ED IN EAX ",
	"EE OUT DX ",
	"EF OUT DX ",
	"F0 LOCK",
	"F1 INT1",
	"F2 REPNE",
	"F3 REPE ECX",
	"F4 HLT",
	"F5 CMC",
	"F6 TEST",
	"F7 IMUL",
	"F8 CLC",
	"F9 STC",
	"FA CLI ",
	"FB STI",
	"FC CLD",
	"FD STD",
	"FE INC",
	"FF DEC"
};

// continue after breakpoint
int
mon_continue(int argc, char **argv, struct Trapframe *tf)
{
	if (tf->tf_trapno != T_BRKPT && tf->tf_trapno != T_DEBUG){
		cprintf("Cannot invoke continue, no breakpoint exception invoked\n");
		return 1;
	}
	uint32_t opcode;
	pte_t *entry;
	uint32_t address;

	// Debug info
	// asm("movl %%ebp, %0"
	//	: "=r"(x)
	// );
	// cprintf("Kernel ebp %08x\n", x);

	if (tf->tf_eflags & FL_TF){
		cprintf("Trap Flag set in EFLAGS\n");
		tf->tf_eflags ^= FL_TF;
	}

	return -1;
}

int
mon_si(int argc, char **argv, struct Trapframe *tf)
{
	if (tf->tf_trapno != T_BRKPT && tf->tf_trapno != T_DEBUG){
		cprintf("Cannot invoke si, no breakpoint exception or debug exception invoked\n");
		return 1;
	}
	uint32_t opcode;
	pte_t *entry;
	uint32_t address;

	// Debug info
	// asm("movl %%ebp, %0"
	//	: "=r"(x)
	// );
	// cprintf("Kernel ebp %08x\n", x);

	address = tf->tf_eip;
	entry = pgdir_walk(curenv->env_pgdir, (void *)address, 0);

	if (entry == NULL){
		panic("Bad address in gdb");
	}

	// Debug info
	// cprintf("Entry addr %x\n", PTE_ADDR(*entry));

	address = (uint32_t)KADDR(PTE_ADDR(*entry)) | (address & 0xfff);

	// Debug info
	// cprintf("Entry addr %x\n", address);
	// cprintf("First byte of eip %x: %x\n", tf->tf_eip, opcode);

	opcode = *((uint32_t *)address);
	opcode &= 0xff;
	cprintf("Instruction: %s\n", opcnames[(int)opcode]);

	// Debug info
	if (tf->tf_eflags & FL_TF){
		cprintf("Trap Flag set in EFLAGS\n");
	}
	tf->tf_eflags |= FL_TF | FL_RF;

	return -1;
}

