#include "cpu.h"
#include "alu.h"
#include "ppu.h"
#include "bus.h"
#include "ops.h"
#include "timer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void cpu_init(struct cpu *cpu, struct bus *bus, struct ppu *ppu,
              struct timer *timer) {
    cpu->regs.af = 0;
    cpu->regs.bc = 0;
    cpu->regs.de = 0;
    cpu->regs.hl = 0;
    cpu->regs.sp = 0;
    cpu->regs.pc = 0;

    cpu->regs.ime = 0;
    cpu->regs.ime_pending = 0;
    cpu->regs.halted = false;

    cpu->cycles = 0;

    cpu->bus = bus;
    cpu->ppu = ppu;
    cpu->timer = timer;
}

void cpu_skip_boot(struct cpu *cpu) {
    cpu->regs.a = 0x01;
    cpu->regs.f = 0xB0;
    cpu->regs.b = 0x00;
    cpu->regs.c = 0x13;
    cpu->regs.d = 0x00;
    cpu->regs.e = 0xD8;
    cpu->regs.h = 0x01;
    cpu->regs.l = 0x4D;
    cpu->regs.pc = 0x0100;
    cpu->regs.sp = 0xFFFE;
    bus_mem_write8(cpu->bus, 0xFF40, 0x91);
    bus_mem_write8(cpu->bus, 0xFF41, 0x85);
    bus_mem_write8(cpu->bus, 0xFF47, 0xFC);
    bus_mem_write8(cpu->bus, 0xFF0F, 0xE1);
    bus_mem_write8(cpu->bus, 0xFFFF, 0x00);
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

static void cpu_tick(struct cpu *cpu) {
    timer_tick(cpu->timer, 4, cpu->bus);
    dma_tick(cpu->bus);
    ppu_tick(cpu->ppu, 4, cpu->bus);
    cpu->cycles += 4;
}

uint8_t cpu_read8(struct cpu *cpu, uint16_t addr) {
    cpu_tick(cpu);
    if (cpu->bus->dma_active && addr < 0xFF80) return 0xFF;
    return bus_mem_read8(cpu->bus, addr);
}

void cpu_write8(struct cpu *cpu, uint16_t addr, uint8_t val) {
    cpu_tick(cpu);
    if (cpu->bus->dma_active && addr < 0xFF80) return;
    bus_mem_write8(cpu->bus, addr, val);
}

void cpu_idle(struct cpu *cpu) { cpu_tick(cpu); }

static uint8_t next_token8(struct cpu *cpu) {
    const uint8_t ope = cpu_read8(cpu, cpu->regs.pc);
    cpu->regs.pc++;
    return ope;
}

static uint16_t next_token16(struct cpu *cpu) {
    const uint8_t lo = next_token8(cpu);
    const uint8_t hi = next_token8(cpu);
    return lo | (hi << 8);
}

static uint8_t handle_interrupt(uint8_t bit, struct cpu *cpu) {
    cpu_idle(cpu);
    cpu->bus->io[0x0F] &= ~(1 << bit);
    cpu->regs.ime = false;
    push(cpu->regs.pc, cpu);
    cpu->regs.pc = 0x40 + bit * 8;
    cpu_idle(cpu);
    return 20;
}

static void cpu_execute(struct cpu *cpu, bool trace) {
    struct cpu_regs *regs = &cpu->regs;
    struct bus *bus = cpu->bus;

    // debug print
    if (trace)
        cpu_step_print(regs, bus);

    // handle halted state
    if (regs->halted && (bus->ie & bus->io[0x0F] & 0x1F))
        regs->halted = false;
    if (regs->halted) {
        cpu_idle(cpu);
        return;
    }

    // handle interrupts
    if (regs->ime)
        for (size_t i = 0; i < 5; i++)
            if ((bus->ie & (1 << i)) && (bus->io[0x0F] & (1 << i))) {
                handle_interrupt(i, cpu);
                return;
            }

    // ime promotion
    if (regs->ime_pending) {
        regs->ime = true;
        regs->ime_pending = false;
    }

    // 1. fetch: read the byte at PC, increment PC
    uint8_t opcode = next_token8(cpu);

    // 2. decode: opcode -> what to do
    switch (opcode) {
    // 3. execute

    // 0x0- ====================================================================
    // NOP
    case 0x00:
        nop();
        break;

    // LD BC,u16
    case 0x01:
        ld_r16_n16(&regs->bc, next_token16(cpu));
        break;

    // LD (BC),A
    case 0x02:
        ld_m_n8(regs->bc, regs->a, cpu);
        break;

    // INC BC
    case 0x03:
        inc_r16(&regs->bc, cpu);
        break;

    // INC B
    case 0x04:
        alu_inc_r8(&regs->b, regs);
        break;

    // DEC B
    case 0x05:
        alu_dec_r8(&regs->b, regs);
        break;

    // LD B,u8
    case 0x06:
        ld_r8_n8(&regs->b, next_token8(cpu));
        break;

    // RLCA
    case 0x07:
        rlca(regs);
        break;

    // LD (a16),SP
    case 0x08:
        ld_m_n16(next_token16(cpu), regs->sp, cpu);
        break;

    // ADD HL,BC
    case 0x09:
        add_r16_n16(&regs->hl, regs->bc, cpu);
        break;

    // LD A,(BC)
    case 0x0A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->bc));
        break;

    // DEC BC
    case 0x0B:
        dec_r16(&regs->bc, cpu);
        break;

    // INC C
    case 0x0C:
        alu_inc_r8(&regs->c, regs);
        break;

    // DEC C
    case 0x0D:
        alu_dec_r8(&regs->c, regs);
        break;

    // LD C,u8
    case 0x0E:
        ld_r8_n8(&regs->c, next_token8(cpu));
        break;

    // RRCA
    case 0x0F:
        rrca(regs);
        break;

    // 0x1- ====================================================================
    // STOP
    case 0x10:
        stop();
        regs->pc++;
        break;

    // LD DE,u16
    case 0x11:
        ld_r16_n16(&regs->de, next_token16(cpu));
        break;

    // LD (DE),A
    case 0x12:
        ld_m_n8(regs->de, regs->a, cpu);
        break;

    // INC DE
    case 0x13:
        inc_r16(&regs->de, cpu);
        break;

    // INC D
    case 0x14:
        alu_inc_r8(&regs->d, regs);
        break;

    // DEC D
    case 0x15:
        alu_dec_r8(&regs->d, regs);
        break;

    // LD d,u8
    case 0x16:
        ld_r8_n8(&regs->d, next_token8(cpu));
        break;

    // LD,u8
    case 0x17:
        rla(regs);
        break;

    // JR i8
    case 0x18:
        jr((int8_t)next_token8(cpu), cpu);
        break;

    // ADD HL,DE
    case 0x19:
        add_r16_n16(&regs->hl, regs->de, cpu);
        break;

    // LD A,(DE)
    case 0x1A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->de));
        break;

    // DEC DE
    case 0x1B:
        dec_r16(&regs->de, cpu);
        break;

    // INC E
    case 0x1C:
        alu_inc_r8(&regs->e, regs);
        break;

    // DEC E
    case 0x1D:
        alu_dec_r8(&regs->e, regs);
        break;

    // LD E,u8
    case 0x1E:
        ld_r8_n8(&regs->e, next_token8(cpu));
        break;

    // RRA
    case 0x1F:
        rra(regs);
        break;

    // 0x2- ====================================================================
    // LD HL,u16
    case 0x21:
        ld_r16_n16(&regs->hl, next_token16(cpu));
        break;

    // LD (HL+),A
    case 0x22:
        ld_m_n8(regs->hl, regs->a, cpu);
        regs->hl++;
        break;

    // INC HL
    case 0x23:
        inc_r16(&regs->hl, cpu);
        break;

    // INC H
    case 0x24:
        alu_inc_r8(&regs->h, regs);
        break;

    // DEC H
    case 0x25:
        alu_dec_r8(&regs->h, regs);
        break;

    // LD H,u8
    case 0x26:
        ld_r8_n8(&regs->h, next_token8(cpu));
        break;

    // DAA
    case 0x27:
        alu_daa(regs);
        break;

    // ADD HL,HL
    case 0x29:
        add_r16_n16(&regs->hl, regs->hl, cpu);
        break;

    // LD A,(HL+)
    case 0x2A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->hl));
        regs->hl++;
        break;

    // DEC HL
    case 0x2B:
        dec_r16(&regs->hl, cpu);
        break;

    // INC L
    case 0x2C:
        alu_inc_r8(&regs->l, regs);
        break;

    // DEC L
    case 0x2D:
        alu_dec_r8(&regs->l, regs);
        break;

    // LD L,u8
    case 0x2E:
        ld_r8_n8(&regs->l, next_token8(cpu));
        break;

    // CPL
    case 0x2F:
        alu_cpl(regs);
        break;

    // 0x3- ====================================================================
    // LD SP,u16
    case 0x31:
        ld_r16_n16(&regs->sp, next_token16(cpu));
        break;

    // LD (HL-),A
    case 0x32:
        ld_m_n8(regs->hl, regs->a, cpu);
        regs->hl--;
        break;

    // INC SP
    case 0x33:
        inc_r16(&regs->sp, cpu);
        break;

    // INC (HL)
    case 0x34:
        inc_m(regs->hl, cpu);
        break;

    // DEC (HL)
    case 0x35:
        dec_m(regs->hl, cpu);
        break;

    // LD (HL),u8
    case 0x36:
        ld_m_n8(regs->hl, next_token8(cpu), cpu);
        break;

    // SCF
    case 0x37:
        alu_scf(regs);
        break;

    // ADD HL,SP
    case 0x39:
        add_r16_n16(&regs->hl, regs->sp, cpu);
        break;

    // LD A,(HL-)
    case 0x3A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->hl));
        regs->hl--;
        break;

    // DEC SP
    case 0x3B:
        dec_r16(&regs->sp, cpu);
        break;

    // INC A
    case 0x3C:
        alu_inc_r8(&regs->a, regs);
        break;

    // DEC A
    case 0x3D:
        alu_dec_r8(&regs->a, regs);
        break;

    // LD A,u8
    case 0x3E:
        ld_r8_n8(&regs->a, next_token8(cpu));
        break;

    // CFF
    case 0x3F:
        alu_ccf(regs);
        break;

    // 0x4- ====================================================================
    // LD B,B
    case 0x40:
        ld_r8_n8(&regs->b, regs->b);
        break;

    // LD B,C
    case 0x41:
        ld_r8_n8(&regs->b, regs->c);
        break;

    // LD B,D
    case 0x42:
        ld_r8_n8(&regs->b, regs->d);
        break;

    // LD B,E
    case 0x43:
        ld_r8_n8(&regs->b, regs->e);
        break;

    // LD B,H
    case 0x44:
        ld_r8_n8(&regs->b, regs->h);
        break;

    // LD B,L
    case 0x45:
        ld_r8_n8(&regs->b, regs->l);
        break;

    // LD B,(HL)
    case 0x46:
        ld_r8_n8(&regs->b, cpu_read8(cpu, regs->hl));
        break;

    // LD B,A
    case 0x47:
        ld_r8_n8(&regs->b, regs->a);
        break;

    // LD C,B
    case 0x48:
        ld_r8_n8(&regs->c, regs->b);
        break;

    // LD C,C
    case 0x49:
        ld_r8_n8(&regs->c, regs->c);
        break;

    // LD C,D
    case 0x4A:
        ld_r8_n8(&regs->c, regs->d);
        break;

    // LD C,E
    case 0x4B:
        ld_r8_n8(&regs->c, regs->e);
        break;

    // LD C,H
    case 0x4C:
        ld_r8_n8(&regs->c, regs->h);
        break;

    // LD C,L
    case 0x4D:
        ld_r8_n8(&regs->c, regs->l);
        break;

    // LD C,(HL)
    case 0x4E:
        ld_r8_n8(&regs->c, cpu_read8(cpu, regs->hl));
        break;

    // LD C,A
    case 0x4F:
        ld_r8_n8(&regs->c, regs->a);
        break;

    // 0x5- ====================================================================
    // LD D,B
    case 0x50:
        ld_r8_n8(&regs->d, regs->b);
        break;

    // LD D,C
    case 0x51:
        ld_r8_n8(&regs->d, regs->c);
        break;

    // LD D,D
    case 0x52:
        ld_r8_n8(&regs->d, regs->d);
        break;

    // LD D,E
    case 0x53:
        ld_r8_n8(&regs->d, regs->e);
        break;

    // LD D,H
    case 0x54:
        ld_r8_n8(&regs->d, regs->h);
        break;

    // LD D,L
    case 0x55:
        ld_r8_n8(&regs->d, regs->l);
        break;

    // LD D,(HL)
    case 0x56:
        ld_r8_n8(&regs->d, cpu_read8(cpu, regs->hl));
        break;

    // LD D,A
    case 0x57:
        ld_r8_n8(&regs->d, regs->a);
        break;

    // LD E,B
    case 0x58:
        ld_r8_n8(&regs->e, regs->b);
        break;

    // LD E,C
    case 0x59:
        ld_r8_n8(&regs->e, regs->c);
        break;

    // LD E,D
    case 0x5A:
        ld_r8_n8(&regs->e, regs->d);
        break;

    // LD E,E
    case 0x5B:
        ld_r8_n8(&regs->e, regs->e);
        break;

    // LD E,H
    case 0x5C:
        ld_r8_n8(&regs->e, regs->h);
        break;

    // LD E,L
    case 0x5D:
        ld_r8_n8(&regs->e, regs->l);
        break;

    // LD E,(HL)
    case 0x5E:
        ld_r8_n8(&regs->e, cpu_read8(cpu, regs->hl));
        break;

    // LD E,A
    case 0x5F:
        ld_r8_n8(&regs->e, regs->a);
        break;

    // 0x6- ====================================================================
    // LD H,B
    case 0x60:
        ld_r8_n8(&regs->h, regs->b);
        break;

    // LD H,C
    case 0x61:
        ld_r8_n8(&regs->h, regs->c);
        break;

    // LD H,D
    case 0x62:
        ld_r8_n8(&regs->h, regs->d);
        break;

    // LD H,E
    case 0x63:
        ld_r8_n8(&regs->h, regs->e);
        break;

    // LD H,H
    case 0x64:
        ld_r8_n8(&regs->h, regs->h);
        break;

    // LD H,L
    case 0x65:
        ld_r8_n8(&regs->h, regs->l);
        break;

    // LD H,(HL)
    case 0x66:
        ld_r8_n8(&regs->h, cpu_read8(cpu, regs->hl));
        break;

    // LD H,A
    case 0x67:
        ld_r8_n8(&regs->h, regs->a);
        break;

    // LD L,B
    case 0x68:
        ld_r8_n8(&regs->l, regs->b);
        break;

    // LD L,C
    case 0x69:
        ld_r8_n8(&regs->l, regs->c);
        break;

    // LD L,D
    case 0x6A:
        ld_r8_n8(&regs->l, regs->d);
        break;

    // LD L,E
    case 0x6B:
        ld_r8_n8(&regs->l, regs->e);
        break;

    // LD L,H
    case 0x6C:
        ld_r8_n8(&regs->l, regs->h);
        break;

    // LD L,L
    case 0x6D:
        ld_r8_n8(&regs->l, regs->l);
        break;

    // LD L,(HL)
    case 0x6E:
        ld_r8_n8(&regs->l, cpu_read8(cpu, regs->hl));
        break;

    // LD L,A
    case 0x6F:
        ld_r8_n8(&regs->l, regs->a);
        break;

    // 0x7- ====================================================================
    // LD (HL),B
    case 0x70:
        ld_m_n8(regs->hl, regs->b, cpu);
        break;

    // LD (HL),C
    case 0x71:
        ld_m_n8(regs->hl, regs->c, cpu);
        break;

    // LD (HL),D
    case 0x72:
        ld_m_n8(regs->hl, regs->d, cpu);
        break;

    // LD (HL),E
    case 0x73:
        ld_m_n8(regs->hl, regs->e, cpu);
        break;

    // LD (HL),H
    case 0x74:
        ld_m_n8(regs->hl, regs->h, cpu);
        break;

    // LD (HL),L
    case 0x75:
        ld_m_n8(regs->hl, regs->l, cpu);
        break;

    // HALT
    case 0x76:
        halt(regs);
        break;

    // LD (HL),A
    case 0x77:
        ld_m_n8(regs->hl, regs->a, cpu);
        break;

    // LD A,B
    case 0x78:
        ld_r8_n8(&regs->a, regs->b);
        break;

    // LD A,C
    case 0x79:
        ld_r8_n8(&regs->a, regs->c);
        break;

    // LD A,D
    case 0x7A:
        ld_r8_n8(&regs->a, regs->d);
        break;

    // LD A,E
    case 0x7B:
        ld_r8_n8(&regs->a, regs->e);
        break;

    // LD A,H
    case 0x7C:
        ld_r8_n8(&regs->a, regs->h);
        break;

    // LD A,L
    case 0x7D:
        ld_r8_n8(&regs->a, regs->l);
        break;

    // LD A,(HL)
    case 0x7E:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->hl));
        break;

    // LD A,A
    case 0x7F:
        ld_r8_n8(&regs->a, regs->a);
        break;

    // 0x8- ====================================================================
    // ADD A,B
    case 0x80:
        alu_add_r8(regs->b, 0, regs);
        break;

    // ADD A,C
    case 0x81:
        alu_add_r8(regs->c, 0, regs);
        break;

    // ADD A,D
    case 0x82:
        alu_add_r8(regs->d, 0, regs);
        break;

    // ADD A,E
    case 0x83:
        alu_add_r8(regs->e, 0, regs);
        break;

    // ADD A,H
    case 0x84:
        alu_add_r8(regs->h, 0, regs);
        break;

    // ADD A,L
    case 0x85:
        alu_add_r8(regs->l, 0, regs);
        break;

    // ADD A,(HL)
    case 0x86:
        alu_add_r8(cpu_read8(cpu, regs->hl), 0, regs);
        break;

    // ADD A,A
    case 0x87:
        alu_add_r8(regs->a, 0, regs);
        break;

    // ADC A,B
    case 0x88:
        alu_add_r8(regs->b, regs->f_bits.c, regs);
        break;

    // ADC A,C
    case 0x89:
        alu_add_r8(regs->c, regs->f_bits.c, regs);
        break;

    // ADC A,D
    case 0x8A:
        alu_add_r8(regs->d, regs->f_bits.c, regs);
        break;

    // ADC A,E
    case 0x8B:
        alu_add_r8(regs->e, regs->f_bits.c, regs);
        break;

    // ADC A,H
    case 0x8C:
        alu_add_r8(regs->h, regs->f_bits.c, regs);
        break;

    // ADC A,L
    case 0x8D:
        alu_add_r8(regs->l, regs->f_bits.c, regs);
        break;

    // ADC A,(HL)
    case 0x8E:
        alu_add_r8(cpu_read8(cpu, regs->hl), regs->f_bits.c, regs);
        break;

    // ADC A,A
    case 0x8F:
        alu_add_r8(regs->a, regs->f_bits.c, regs);
        break;

    // 0x9- ====================================================================
    // SUB A,B
    case 0x90:
        alu_sub_r8(regs->b, 0, regs);
        break;

    // SUB A,C
    case 0x91:
        alu_sub_r8(regs->c, 0, regs);
        break;

    // SUB A,D
    case 0x92:
        alu_sub_r8(regs->d, 0, regs);
        break;

    // SUB A,E
    case 0x93:
        alu_sub_r8(regs->e, 0, regs);
        break;

    // SUB A,H
    case 0x94:
        alu_sub_r8(regs->h, 0, regs);
        break;

    // SUB A,L
    case 0x95:
        alu_sub_r8(regs->l, 0, regs);
        break;

    // SUB A,(HL)
    case 0x96:
        alu_sub_r8(cpu_read8(cpu, regs->hl), 0, regs);
        break;

    // SUB A,A
    case 0x97:
        alu_sub_r8(regs->a, 0, regs);
        break;

    // SBC A,B
    case 0x98:
        alu_sub_r8(regs->b, regs->f_bits.c, regs);
        break;

    // SBC A,C
    case 0x99:
        alu_sub_r8(regs->c, regs->f_bits.c, regs);
        break;

    // SBC A,D
    case 0x9A:
        alu_sub_r8(regs->d, regs->f_bits.c, regs);
        break;

    // SBC A,E
    case 0x9B:
        alu_sub_r8(regs->e, regs->f_bits.c, regs);
        break;

    // SBC A,H
    case 0x9C:
        alu_sub_r8(regs->h, regs->f_bits.c, regs);
        break;

    // SBC A,L
    case 0x9D:
        alu_sub_r8(regs->l, regs->f_bits.c, regs);
        break;

    // SBC A,(HL)
    case 0x9E:
        alu_sub_r8(cpu_read8(cpu, regs->hl), regs->f_bits.c, regs);
        break;

    // SBC A,A
    case 0x9F:
        alu_sub_r8(regs->a, regs->f_bits.c, regs);
        break;

    // 0xA- ====================================================================
    // AND A,B
    case 0xA0:
        alu_and(regs->b, regs);
        break;

    // AND A,C
    case 0xA1:
        alu_and(regs->c, regs);
        break;

    // AND A,D
    case 0xA2:
        alu_and(regs->d, regs);
        break;

    // AND A,E
    case 0xA3:
        alu_and(regs->e, regs);
        break;

    // AND A,H
    case 0xA4:
        alu_and(regs->h, regs);
        break;

    // AND A,L
    case 0xA5:
        alu_and(regs->l, regs);
        break;

    // AND A,(HL)
    case 0xA6:
        alu_and(cpu_read8(cpu, regs->hl), regs);
        break;

    // AND A,A
    case 0xA7:
        alu_and(regs->a, regs);
        break;

    // XOR A,B
    case 0xA8:
        alu_xor(regs->b, regs);
        break;

    // XOR A,C
    case 0xA9:
        alu_xor(regs->c, regs);
        break;

    // XOR A,D
    case 0xAA:
        alu_xor(regs->d, regs);
        break;

    // XOR A,E
    case 0xAB:
        alu_xor(regs->e, regs);
        break;

    // XOR A,H
    case 0xAC:
        alu_xor(regs->h, regs);
        break;

    // XOR A,L
    case 0xAD:
        alu_xor(regs->l, regs);
        break;

    // XOR A,(HL)
    case 0xAE:
        alu_xor(cpu_read8(cpu, regs->hl), regs);
        break;

    // XOR A,A
    case 0xAF:
        alu_xor(regs->a, regs);
        break;

    // 0xB- ====================================================================
    // OR A,B
    case 0xB0:
        alu_or(regs->b, regs);
        break;

    // OR A,C
    case 0xB1:
        alu_or(regs->c, regs);
        break;

    // OR A,D
    case 0xB2:
        alu_or(regs->d, regs);
        break;

    // OR A,E
    case 0xB3:
        alu_or(regs->e, regs);
        break;

    // OR A,H
    case 0xB4:
        alu_or(regs->h, regs);
        break;

    // OR A,L
    case 0xB5:
        alu_or(regs->l, regs);
        break;

    // OR A,(HL)
    case 0xB6:
        alu_or(cpu_read8(cpu, regs->hl), regs);
        break;

    // OR A,A
    case 0xB7:
        alu_or(regs->a, regs);
        break;

    // CP A,B
    case 0xB8:
        alu_cp(regs->b, regs);
        break;

    // CP A,C
    case 0xB9:
        alu_cp(regs->c, regs);
        break;

    // CP A,D
    case 0xBA:
        alu_cp(regs->d, regs);
        break;

    // CP A,E
    case 0xBB:
        alu_cp(regs->e, regs);
        break;

    // cp A,H
    case 0xBC:
        alu_cp(regs->h, regs);
        break;

    // cp A,L
    case 0xBD:
        alu_cp(regs->l, regs);
        break;

    // CP A,(HL)
    case 0xBE:
        alu_cp(cpu_read8(cpu, regs->hl), regs);
        break;

    // CP A,A
    case 0xBF:
        alu_cp(regs->a, regs);
        break;

    // 0xC- ====================================================================
    // POP BC
    case 0xC1:
        pop(&regs->bc, cpu);
        break;

    // JP n16
    case 0xC3:
        jp(next_token16(cpu), cpu);
        break;

    // PUSH BC
    case 0xC5:
        push(regs->bc, cpu);
        break;

    // ADD A,u8
    case 0xC6:
        alu_add_r8(next_token8(cpu), 0, regs);
        break;

    // RET
    case 0xC9:
        ret(cpu);
        break;

    // PREFIX CB
    case 0xCB:
        cpu_step_cb(next_token8(cpu), cpu);
        break;

    // ADC A,u8
    case 0xCE:
        alu_add_r8(next_token8(cpu), regs->f_bits.c, regs);
        break;

    // CALL u16
    case 0xCD:
        call(next_token16(cpu), cpu);
        break;

    // 0xD- ====================================================================
    // POP DE
    case 0xD1:
        pop(&regs->de, cpu);
        break;

    // PUSH DE
    case 0xD5:
        push(regs->de, cpu);
        break;

    // SUB A,u8
    case 0xD6:
        alu_sub_r8(next_token8(cpu), 0, regs);
        break;

    // RETI
    case 0xD9:
        reti(cpu);
        break;

    // SBC A,u8
    case 0xDE:
        alu_sub_r8(next_token8(cpu), regs->f_bits.c, regs);
        break;

    // 0xE- ====================================================================
    // LDH (n8),A
    case 0xE0:
        ld_m_n8(0xFF00 + next_token8(cpu), regs->a, cpu);
        break;

    // POP HL
    case 0xE1:
        pop(&regs->hl, cpu);
        break;

    // LDH (c),A
    case 0xE2:
        ld_m_n8(0xFF00 + regs->c, regs->a, cpu);
        break;

    // PUSH HL
    case 0xE5:
        push(regs->hl, cpu);
        break;

    // AND A,u8
    case 0xE6:
        alu_and(next_token8(cpu), regs);
        break;

    // ADD SP,u8
    case 0xE8:
        add_sp((int8_t)next_token8(cpu), cpu);
        break;

    // JP HL
    case 0xE9:
        regs->pc = regs->hl; // no cycles
        break;

    // LD (u16),A
    case 0xEA:
        ld_m_n8(next_token16(cpu), regs->a, cpu);
        break;

    // XOR A,u8
    case 0xEE:
        alu_xor(next_token8(cpu), regs);
        break;

    // 0xF- ====================================================================
    // LDH a,(n8)
    case 0xF0:
        ld_r8_n8(&regs->a, cpu_read8(cpu, 0xFF00 + next_token8(cpu)));
        break;

    // POP AF
    case 0xF1:
        pop(&regs->af, cpu);
        regs->f &= 0xF0;
        break;

    // LDH A,(c)
    case 0xF2:
        ld_r8_n8(&regs->a, cpu_read8(cpu, 0xFF00 + regs->c));
        break;

    // DI
    case 0xF3:
        di(regs);
        break;

    // PUSH AF
    case 0xF5:
        push(regs->af, cpu);
        break;

    // OR A,u8
    case 0xF6:
        alu_or(next_token8(cpu), regs);
        break;

    // LD HL,SP+i8
    case 0xF8:
        ld_hl_spe((int8_t)next_token8(cpu), cpu);
        break;

    // LD SP,HL
    case 0xF9:
        ld_r16_n16(&regs->sp, regs->hl);
        cpu_idle(cpu);
        break;

    // LD A,(u16)
    case 0xFA:
        ld_r8_n8(&regs->a, cpu_read8(cpu, next_token16(cpu)));
        break;

    // EI
    case 0xFB:
        ei(regs);
        break;

    // CP A,n8
    case 0xFE:
        alu_cp(next_token8(cpu), regs);
        break;

    // -------------------------------------------------------------------------
    // JUMPS
    // JR cc,i8
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38: {
        const uint8_t operand = next_token8(cpu);
        if (cond_met(opcode, regs))
            jr((int8_t)operand, cpu);
        break;
    }
    // JP cc,a16
    case 0xC2:
    case 0xCA:
    case 0xD2:
    case 0xDA: {
        const uint16_t operand = next_token16(cpu);
        if (cond_met(opcode, regs))
            jp(operand, cpu);
        break;
    }

    // CALL cc/RET cc/RST
    case 0xC4:
    case 0xCC:
    case 0xD4:
    case 0xDC: {
        const uint16_t operand = next_token16(cpu);
        if (cond_met(opcode, regs))
            call(operand, cpu);
        break;
    }
    case 0xC0:
    case 0xC8:
    case 0xD0:
    case 0xD8:
        cpu_idle(cpu);
        if (cond_met(opcode, regs))
            ret(cpu);
        break;
    case 0xC7:
    case 0xCF:
    case 0xD7:
    case 0xDF:
    case 0xE7:
    case 0xEF:
    case 0xF7:
    case 0xFF:
        call(opcode & 0x38, cpu);
        break;

    default:
        fprintf(stderr, "error opcode: %02X\n", opcode);
        fprintf(stderr, "error pc    : %04X\n", (unsigned int)(regs->pc - 1));
        exit(1);
    }
    
}

void cpu_step(struct cpu *cpu, bool trace) {
    cpu_execute(cpu, trace);
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
