#include "cpu.h"
#include "bus.h"
#include "ops.h"
#include <stdio.h>
#include <stdlib.h>

void cpu_init(struct cpu_regs *regs) {
    regs->af = 0;
    regs->bc = 0;
    regs->de = 0;
    regs->hl = 0;
    regs->sp = 0;
    regs->pc = 0;
}

void cpu_skip_boot(struct cpu_regs *regs, struct bus *bus) {
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
    bus_mem_write8(bus, 0xFF40, 0x91);
    bus_mem_write8(bus, 0xFF41, 0x85);
    bus_mem_write8(bus, 0xFF47, 0xFC);
    bus_mem_write8(bus, 0xFF0F, 0xE1);
    bus_mem_write8(bus, 0xFFFF, 0x00);
}

static bool cond_met(uint8_t opcode, struct cpu_regs *regs) {
    switch ((opcode >> 3) & 0x3) {
    case 0:
        return !regs->f_bits.z;
    case 1:
        return regs->f_bits.z;
    case 2:
        return !regs->f_bits.c;
    case 3:
        return regs->f_bits.c;
    default:
        return false;
    };
}

uint8_t cpu_step(struct cpu_regs *regs, struct bus *bus, bool trace) {
    if (trace)
        cpu_step_print(regs, bus);

    // 1. fetch: read the byte at PC, increment PC
    uint8_t opcode = bus_mem_read8(bus, regs->pc++);

    // 2. decode: opcode -> what to do
    switch (opcode) {
    // 3. execute

    // NOP
    case 0x00:
        nop();
        return 4;

    // LD BC,u16
    case 0x01:
        ld_r16_n16(&regs->bc, bus_mem_read16(bus, regs->pc));
        regs->pc += 2;
        return 12;

    // LD (BC),A
    case 0x02:
        ld_m_n8(regs->bc, regs->a, bus);
        return 8;

    // INC BC
    case 0x03:
        inc_r16(&regs->bc);
        return 8;

    // INC B
    case 0x04:
        inc_r8(&regs->b, regs);
        return 4;

    // DEC B
    case 0x05:
        dec_r8(&regs->b, regs);
        return 4;

    // LD B,u8
    case 0x06:
        ld_r8_n8(&regs->b, bus_mem_read8(bus, regs->pc));
        regs->pc++;
        return 8;

    // RLCA
    case 0x07:
        rlca(regs);
        return 4;

    // LD (a16),SP
    case 0x08:
        ld_m_n16(bus_mem_read16(bus, regs->pc), regs->sp, bus);
        regs->pc += 2;
        return 20;

    // ADD HL,BC
    case 0x09:
        add_r16_n16(&regs->hl, regs->bc, regs);
        return 8;

    // LD A,(BC)
    case 0x0A:
        ld_r8_n8(&regs->a, bus_mem_read8(bus, regs->bc));
        return 8;

    // DEC BC
    case 0x0B:
        dec_r16(&regs->bc);
        return 8;

    // INC C
    case 0x0C:
        inc_r8(&regs->c, regs);
        return 4;

    // DEC C
    case 0x0D:
        dec_r8(&regs->c, regs);
        return 4;

    // LD C,u8
    case 0x0E:
        ld_r8_n8(&regs->c, bus_mem_read8(bus, regs->pc));
        regs->pc++;
        return 8;

    // RRCA
    case 0x0F:
        rrca(regs);
        return 4;

    // STOP
    case 0x10:
        stop();
        regs->pc++;
        return 4;

    // LD DE,u16
    case 0x11:
        ld_r16_n16(&regs->de, bus_mem_read16(bus, regs->pc));
        regs->pc += 2;
        return 12;

    // LD (DE),A
    case 0x12:
        ld_m_n8(regs->de, regs->a, bus);
        return 8;

    // INC DE
    case 0x13:
        inc_r16(&regs->de);
        return 8;

    // -------------------------------------------------------------------------

    // JP n16
    case 0xC3:
        ld_r16_n16(&regs->pc, bus_mem_read16(bus, regs->pc));
        return 16;

    // JR cc,i8
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38: {
        const int8_t off = bus_mem_read8(bus, regs->pc);
        regs->pc++;
        if (cond_met(opcode, regs)) {
            jr_e8(off, regs);
            return 12;
        }
        return 8;
    }

    // LD HL,n16
    case 0x21:
        ld_r16_n16(&regs->hl, bus_mem_read16(bus, regs->pc));
        regs->pc += 2;
        return 12;

    // LD A,(HL+)
    case 0x2A:
        ld_r8_n8(&regs->a, bus_mem_read8(bus, regs->hl));
        regs->hl++;
        return 8;

    // INC A
    case 0x3C:
        inc_r8(&regs->a, regs);
        return 4;

    // LD A,B
    case 0x78:
        ld_r8_n8(&regs->a, regs->b);
        return 4;

    // OR A,C
    case 0xB1:
        or_n8(regs->c, regs);
        return 4;

    // LDH (n8), A
    case 0xE0:
        ld_m_n8(0xFF00 + bus_mem_read8(bus, regs->pc), regs->a, bus);
        regs->pc++;
        return 12;

    // JR i8
    case 0x18: {
        int8_t off = (int8_t)bus_mem_read8(bus, regs->pc++);
        jr_e8(off, regs);
        return 12;
    }

    // LD B,A
    case 0x47:
        ld_r8_n8(&regs->b, regs->a);
        return 4;

    // LD A,u8
    case 0x3E:
        ld_r8_n8(&regs->a, bus_mem_read8(bus, regs->pc));
        regs->pc++;
        return 8;

    // LDH a,(n8)
    case 0xF0:
        ld_r8_n8(&regs->a,
                 bus_mem_read8(bus, 0xFF00 + bus_mem_read8(bus, regs->pc)));
        regs->pc++;
        return 12;

    // CP n8
    case 0xFE:
        cp_n8(bus_mem_read8(bus, regs->pc), regs);
        regs->pc++;
        return 8;

    default:
        fprintf(stderr, "error opcode: %02X\n", opcode);
        fprintf(stderr, "error pc    : %04X\n", (unsigned int)(regs->pc - 1));
        exit(1);
    }
}

void cpu_regs_print(struct cpu_regs *regs) {
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

void cpu_step_print(struct cpu_regs *regs, struct bus *bus) {
    printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X "
           "SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
           regs->a, regs->f, regs->b, regs->c, regs->d, regs->e, regs->h,
           regs->l, regs->sp, regs->pc, bus_mem_read8(bus, regs->pc + 0),
           bus_mem_read8(bus, regs->pc + 1), bus_mem_read8(bus, regs->pc + 2),
           bus_mem_read8(bus, regs->pc + 3));
}
