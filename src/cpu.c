#include "cpu.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

void cpu_init(struct cpu_registers *regs) {
    regs->af = 0;
    regs->bc = 0;
    regs->de = 0;
    regs->hl = 0;
    regs->sp = 0;
    regs->pc = 0;
}

void cpu_skip_boot(struct cpu_registers *regs, struct memory *mem) {
    regs->a = 0x01;
    regs->f = 0xB0;
    regs->b = 0x00;
    regs->c = 0x13;
    regs->d = 0x00;
    regs->e = 0xD8;
    regs->h = 0x01;
    regs->l = 0x4D;
    regs->pc = 0x0100;
    regs->sp = 0xFFFE;
    mem_write(mem, 0xFF40, 0x91);
    mem_write(mem, 0xFF41, 0x85);
    mem_write(mem, 0xFF47, 0xFC);
    mem_write(mem, 0xFF0F, 0xE1);
    mem_write(mem, 0xFFFF, 0x00);
}

uint8_t cpu_step(struct cpu_registers *regs, struct memory *mem) {
    // 1. fetch: read the byte at PC, increment PC
    uint8_t opcode = mem_read(mem, regs->pc++);

    // 2. decode: opcode -> what to do
    switch (opcode) {
    // 3. execute: do
    case 0x00: // NOP
        fprintf(stdout, "%04X: %02X = NOP\n",
                (unsigned int)(regs->pc - 1), opcode);
        return 4;
    default:
        fprintf(stderr, "opcode: %02X\n", opcode);
        fprintf(stderr, "pc    : %04X\n", (unsigned int)(regs->pc - 1));
        exit(1);
    }
}

void cpu_registers_print(struct cpu_registers *regs) {
    fprintf(stdout,
            "AF: %04X  (A: %02X  F: %02X)\n"
            "BC: %04X  (B: %02X  C: %02X)\n"
            "DE: %04X  (D: %02X  E: %02X)\n"
            "HL: %04X  (H: %02X  L: %02X)\n"
            "SP: %04X\n"
            "PC: %04X\n"
            "Flags: Z=%u N=%u H=%u C=%u\n",
            regs->af, regs->a, regs->f, regs->bc, regs->b, regs->c, regs->de,
            regs->d, regs->e, regs->hl, regs->h, regs->l, regs->sp, regs->pc,
            regs->f_bits.z, regs->f_bits.n, regs->f_bits.h, regs->f_bits.c);
}
