#include "cpu.h"
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
    ppu_tick(cpu->ppu, 4, cpu->bus);
    cpu->cycles += 4;
}

uint8_t cpu_read8(struct cpu *cpu, uint16_t addr) {
    cpu_tick(cpu);
    return bus_mem_read8(cpu->bus, addr);
}

void cpu_write8(struct cpu *cpu, uint16_t addr, uint8_t val) {
    cpu_tick(cpu);
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

static uint8_t cpu_execute(struct cpu *cpu, bool trace) {
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
        return 4;
    }

    // handle interrupts
    if (regs->ime)
        for (size_t i = 0; i < 5; i++)
            if ((bus->ie & (1 << i)) && (bus->io[0x0F] & (1 << i)))
                return handle_interrupt(i, cpu);

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
        return 4;

    // LD BC,u16
    case 0x01:
        ld_r16_n16(&regs->bc, next_token16(cpu));
        return 12;

    // LD (BC),A
    case 0x02:
        ld_m_n8(regs->bc, regs->a, cpu);
        return 8;

    // INC BC
    case 0x03:
        inc_r16(&regs->bc, cpu);
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
        ld_r8_n8(&regs->b, next_token8(cpu));
        return 8;

    // RLCA
    case 0x07:
        rlca(regs);
        return 4;

    // LD (a16),SP
    case 0x08:
        ld_m_n16(next_token16(cpu), regs->sp, cpu);
        return 20;

    // ADD HL,BC
    case 0x09:
        add_r16_n16(&regs->hl, regs->bc, cpu);
        return 8;

    // LD A,(BC)
    case 0x0A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->bc));
        return 8;

    // DEC BC
    case 0x0B:
        dec_r16(&regs->bc, cpu);
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
        ld_r8_n8(&regs->c, next_token8(cpu));
        return 8;

    // RRCA
    case 0x0F:
        rrca(regs);
        return 4;

    // 0x1- ====================================================================
    // STOP
    case 0x10:
        stop();
        regs->pc++;
        return 4;

    // LD DE,u16
    case 0x11:
        ld_r16_n16(&regs->de, next_token16(cpu));
        return 12;

    // LD (DE),A
    case 0x12:
        ld_m_n8(regs->de, regs->a, cpu);
        return 8;

    // INC DE
    case 0x13:
        inc_r16(&regs->de, cpu);
        return 8;

    // INC D
    case 0x14:
        inc_r8(&regs->d, regs);
        return 4;

    // DEC D
    case 0x15:
        dec_r8(&regs->d, regs);
        return 4;

    // LD d,u8
    case 0x16:
        ld_r8_n8(&regs->d, next_token8(cpu));
        return 8;

    // LD,u8
    case 0x17:
        rla(regs);
        return 4;

    // JR i8
    case 0x18:
        jr((int8_t)next_token8(cpu), cpu);
        return 12;

    // ADD HL,DE
    case 0x19:
        add_r16_n16(&regs->hl, regs->de, cpu);
        return 8;

    // LD A,(DE)
    case 0x1A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->de));
        return 8;

    // DEC DE
    case 0x1B:
        dec_r16(&regs->de, cpu);
        return 8;

    // INC E
    case 0x1C:
        inc_r8(&regs->e, regs);
        return 4;

    // DEC E
    case 0x1D:
        dec_r8(&regs->e, regs);
        return 4;

    // LD E,u8
    case 0x1E:
        ld_r8_n8(&regs->e, next_token8(cpu));
        return 8;

    // RRA
    case 0x1F:
        rra(regs);
        return 4;

    // 0x2- ====================================================================
    // LD HL,u16
    case 0x21:
        ld_r16_n16(&regs->hl, next_token16(cpu));
        return 12;

    // LD (HL+),A
    case 0x22:
        ld_m_n8(regs->hl, regs->a, cpu);
        regs->hl++;
        return 8;

    // INC HL
    case 0x23:
        inc_r16(&regs->hl, cpu);
        return 8;

    // INC H
    case 0x24:
        inc_r8(&regs->h, regs);
        return 4;

    // DEC H
    case 0x25:
        dec_r8(&regs->h, regs);
        return 4;

    // LD H,u8
    case 0x26:
        ld_r8_n8(&regs->h, next_token8(cpu));
        return 8;

    // DAA
    case 0x27:
        daa(regs);
        return 4;

    // ADD HL,HL
    case 0x29:
        add_r16_n16(&regs->hl, regs->hl, cpu);
        return 8;

    // LD A,(HL+)
    case 0x2A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->hl));
        regs->hl++;
        return 8;

    // DEC HL
    case 0x2B:
        dec_r16(&regs->hl, cpu);
        return 8;

    // INC L
    case 0x2C:
        inc_r8(&regs->l, regs);
        return 4;

    // DEC L
    case 0x2D:
        dec_r8(&regs->l, regs);
        return 4;

    // LD L,u8
    case 0x2E:
        ld_r8_n8(&regs->l, next_token8(cpu));
        return 8;

    // CPL
    case 0x2F:
        cpl(regs);
        return 4;

    // 0x3- ====================================================================
    // LD SP,u16
    case 0x31:
        ld_r16_n16(&regs->sp, next_token16(cpu));
        return 12;

    // LD (HL-),A
    case 0x32:
        ld_m_n8(regs->hl, regs->a, cpu);
        regs->hl--;
        return 8;

    // INC SP
    case 0x33:
        inc_r16(&regs->sp, cpu);
        return 8;

    // INC (HL)
    case 0x34:
        inc_m(regs->hl, cpu);
        return 12;

    // DEC (HL)
    case 0x35:
        dec_m(regs->hl, cpu);
        return 12;

    // LD (HL),u8
    case 0x36:
        ld_m_n8(regs->hl, next_token8(cpu), cpu);
        return 12;

    // SCF
    case 0x37:
        scf(regs);
        return 4;

    // ADD HL,SP
    case 0x39:
        add_r16_n16(&regs->hl, regs->sp, cpu);
        return 8;

    // LD A,(HL-)
    case 0x3A:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->hl));
        regs->hl--;
        return 8;

    // DEC SP
    case 0x3B:
        dec_r16(&regs->sp, cpu);
        return 8;

    // INC A
    case 0x3C:
        inc_r8(&regs->a, regs);
        return 4;

    // DEC A
    case 0x3D:
        dec_r8(&regs->a, regs);
        return 4;

    // LD A,u8
    case 0x3E:
        ld_r8_n8(&regs->a, next_token8(cpu));
        return 8;

    // CFF
    case 0x3F:
        ccf(regs);
        return 4;

    // 0x4- ====================================================================
    // LD B,B
    case 0x40:
        ld_r8_n8(&regs->b, regs->b);
        return 4;

    // LD B,C
    case 0x41:
        ld_r8_n8(&regs->b, regs->c);
        return 4;

    // LD B,D
    case 0x42:
        ld_r8_n8(&regs->b, regs->d);
        return 4;

    // LD B,E
    case 0x43:
        ld_r8_n8(&regs->b, regs->e);
        return 4;

    // LD B,H
    case 0x44:
        ld_r8_n8(&regs->b, regs->h);
        return 4;

    // LD B,L
    case 0x45:
        ld_r8_n8(&regs->b, regs->l);
        return 4;

    // LD B,(HL)
    case 0x46:
        ld_r8_n8(&regs->b, cpu_read8(cpu, regs->hl));
        return 8;

    // LD B,A
    case 0x47:
        ld_r8_n8(&regs->b, regs->a);
        return 4;

    // LD C,B
    case 0x48:
        ld_r8_n8(&regs->c, regs->b);
        return 4;

    // LD C,C
    case 0x49:
        ld_r8_n8(&regs->c, regs->c);
        return 4;

    // LD C,D
    case 0x4A:
        ld_r8_n8(&regs->c, regs->d);
        return 4;

    // LD C,E
    case 0x4B:
        ld_r8_n8(&regs->c, regs->e);
        return 4;

    // LD C,H
    case 0x4C:
        ld_r8_n8(&regs->c, regs->h);
        return 4;

    // LD C,L
    case 0x4D:
        ld_r8_n8(&regs->c, regs->l);
        return 4;

    // LD C,(HL)
    case 0x4E:
        ld_r8_n8(&regs->c, cpu_read8(cpu, regs->hl));
        return 8;

    // LD C,A
    case 0x4F:
        ld_r8_n8(&regs->c, regs->a);
        return 4;

    // 0x5- ====================================================================
    // LD D,B
    case 0x50:
        ld_r8_n8(&regs->d, regs->b);
        return 4;

    // LD D,C
    case 0x51:
        ld_r8_n8(&regs->d, regs->c);
        return 4;

    // LD D,D
    case 0x52:
        ld_r8_n8(&regs->d, regs->d);
        return 4;

    // LD D,E
    case 0x53:
        ld_r8_n8(&regs->d, regs->e);
        return 4;

    // LD D,H
    case 0x54:
        ld_r8_n8(&regs->d, regs->h);
        return 4;

    // LD D,L
    case 0x55:
        ld_r8_n8(&regs->d, regs->l);
        return 4;

    // LD D,(HL)
    case 0x56:
        ld_r8_n8(&regs->d, cpu_read8(cpu, regs->hl));
        return 8;

    // LD D,A
    case 0x57:
        ld_r8_n8(&regs->d, regs->a);
        return 4;

    // LD E,B
    case 0x58:
        ld_r8_n8(&regs->e, regs->b);
        return 4;

    // LD E,C
    case 0x59:
        ld_r8_n8(&regs->e, regs->c);
        return 4;

    // LD E,D
    case 0x5A:
        ld_r8_n8(&regs->e, regs->d);
        return 4;

    // LD E,E
    case 0x5B:
        ld_r8_n8(&regs->e, regs->e);
        return 4;

    // LD E,H
    case 0x5C:
        ld_r8_n8(&regs->e, regs->h);
        return 4;

    // LD E,L
    case 0x5D:
        ld_r8_n8(&regs->e, regs->l);
        return 4;

    // LD E,(HL)
    case 0x5E:
        ld_r8_n8(&regs->e, cpu_read8(cpu, regs->hl));
        return 8;

    // LD E,A
    case 0x5F:
        ld_r8_n8(&regs->e, regs->a);
        return 4;

    // 0x6- ====================================================================
    // LD H,B
    case 0x60:
        ld_r8_n8(&regs->h, regs->b);
        return 4;

    // LD H,C
    case 0x61:
        ld_r8_n8(&regs->h, regs->c);
        return 4;

    // LD H,D
    case 0x62:
        ld_r8_n8(&regs->h, regs->d);
        return 4;

    // LD H,E
    case 0x63:
        ld_r8_n8(&regs->h, regs->e);
        return 4;

    // LD H,H
    case 0x64:
        ld_r8_n8(&regs->h, regs->h);
        return 4;

    // LD H,L
    case 0x65:
        ld_r8_n8(&regs->h, regs->l);
        return 4;

    // LD H,(HL)
    case 0x66:
        ld_r8_n8(&regs->h, cpu_read8(cpu, regs->hl));
        return 8;

    // LD H,A
    case 0x67:
        ld_r8_n8(&regs->h, regs->a);
        return 4;

    // LD L,B
    case 0x68:
        ld_r8_n8(&regs->l, regs->b);
        return 4;

    // LD L,C
    case 0x69:
        ld_r8_n8(&regs->l, regs->c);
        return 4;

    // LD L,D
    case 0x6A:
        ld_r8_n8(&regs->l, regs->d);
        return 4;

    // LD L,E
    case 0x6B:
        ld_r8_n8(&regs->l, regs->e);
        return 4;

    // LD L,H
    case 0x6C:
        ld_r8_n8(&regs->l, regs->h);
        return 4;

    // LD L,L
    case 0x6D:
        ld_r8_n8(&regs->l, regs->l);
        return 4;

    // LD L,(HL)
    case 0x6E:
        ld_r8_n8(&regs->l, cpu_read8(cpu, regs->hl));
        return 8;

    // LD L,A
    case 0x6F:
        ld_r8_n8(&regs->l, regs->a);
        return 4;

    // 0x7- ====================================================================
    // LD (HL),B
    case 0x70:
        ld_m_n8(regs->hl, regs->b, cpu);
        return 8;

    // LD (HL),C
    case 0x71:
        ld_m_n8(regs->hl, regs->c, cpu);
        return 8;

    // LD (HL),D
    case 0x72:
        ld_m_n8(regs->hl, regs->d, cpu);
        return 8;

    // LD (HL),E
    case 0x73:
        ld_m_n8(regs->hl, regs->e, cpu);
        return 8;

    // LD (HL),H
    case 0x74:
        ld_m_n8(regs->hl, regs->h, cpu);
        return 8;

    // LD (HL),L
    case 0x75:
        ld_m_n8(regs->hl, regs->l, cpu);
        return 8;

    // HALT
    case 0x76:
        halt(regs);
        return 4;

    // LD (HL),A
    case 0x77:
        ld_m_n8(regs->hl, regs->a, cpu);
        return 8;

    // LD A,B
    case 0x78:
        ld_r8_n8(&regs->a, regs->b);
        return 4;

    // LD A,C
    case 0x79:
        ld_r8_n8(&regs->a, regs->c);
        return 4;

    // LD A,D
    case 0x7A:
        ld_r8_n8(&regs->a, regs->d);
        return 4;

    // LD A,E
    case 0x7B:
        ld_r8_n8(&regs->a, regs->e);
        return 4;

    // LD A,H
    case 0x7C:
        ld_r8_n8(&regs->a, regs->h);
        return 4;

    // LD A,L
    case 0x7D:
        ld_r8_n8(&regs->a, regs->l);
        return 4;

    // LD A,(HL)
    case 0x7E:
        ld_r8_n8(&regs->a, cpu_read8(cpu, regs->hl));
        return 8;

    // LD A,A
    case 0x7F:
        ld_r8_n8(&regs->a, regs->a);
        return 4;

    // 0x8- ====================================================================
    // ADD A,B
    case 0x80:
        add_n8(regs->b, regs);
        return 4;

    // ADD A,C
    case 0x81:
        add_n8(regs->c, regs);
        return 4;

    // ADD A,D
    case 0x82:
        add_n8(regs->d, regs);
        return 4;

    // ADD A,E
    case 0x83:
        add_n8(regs->e, regs);
        return 4;

    // ADD A,H
    case 0x84:
        add_n8(regs->h, regs);
        return 4;

    // ADD A,L
    case 0x85:
        add_n8(regs->l, regs);
        return 4;

    // ADD A,(HL)
    case 0x86:
        add_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // ADD A,A
    case 0x87:
        add_n8(regs->a, regs);
        return 4;

    // ADC A,B
    case 0x88:
        adc_n8(regs->b, regs);
        return 4;

    // ADC A,C
    case 0x89:
        adc_n8(regs->c, regs);
        return 4;

    // ADC A,D
    case 0x8A:
        adc_n8(regs->d, regs);
        return 4;

    // ADC A,E
    case 0x8B:
        adc_n8(regs->e, regs);
        return 4;

    // ADC A,H
    case 0x8C:
        adc_n8(regs->h, regs);
        return 4;

    // ADC A,L
    case 0x8D:
        adc_n8(regs->l, regs);
        return 4;

    // ADC A,(HL)
    case 0x8E:
        adc_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // ADC A,A
    case 0x8F:
        adc_n8(regs->a, regs);
        return 4;

    // 0x9- ====================================================================
    // SUB A,B
    case 0x90:
        sub_n8(regs->b, regs);
        return 4;

    // SUB A,C
    case 0x91:
        sub_n8(regs->c, regs);
        return 4;

    // SUB A,D
    case 0x92:
        sub_n8(regs->d, regs);
        return 4;

    // SUB A,E
    case 0x93:
        sub_n8(regs->e, regs);
        return 4;

    // SUB A,H
    case 0x94:
        sub_n8(regs->h, regs);
        return 4;

    // SUB A,L
    case 0x95:
        sub_n8(regs->l, regs);
        return 4;

    // SUB A,(HL)
    case 0x96:
        sub_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // SUB A,A
    case 0x97:
        sub_n8(regs->a, regs);
        return 4;

    // SBC A,B
    case 0x98:
        sbc_n8(regs->b, regs);
        return 4;

    // SBC A,C
    case 0x99:
        sbc_n8(regs->c, regs);
        return 4;

    // SBC A,D
    case 0x9A:
        sbc_n8(regs->d, regs);
        return 4;

    // SBC A,E
    case 0x9B:
        sbc_n8(regs->e, regs);
        return 4;

    // SBC A,H
    case 0x9C:
        sbc_n8(regs->h, regs);
        return 4;

    // SBC A,L
    case 0x9D:
        sbc_n8(regs->l, regs);
        return 4;

    // SBC A,(HL)
    case 0x9E:
        sbc_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // SBC A,A
    case 0x9F:
        sbc_n8(regs->a, regs);
        return 4;

    // 0xA- ====================================================================
    // AND A,B
    case 0xA0:
        and_n8(regs->b, regs);
        return 4;

    // AND A,C
    case 0xA1:
        and_n8(regs->c, regs);
        return 4;

    // AND A,D
    case 0xA2:
        and_n8(regs->d, regs);
        return 4;

    // AND A,E
    case 0xA3:
        and_n8(regs->e, regs);
        return 4;

    // AND A,H
    case 0xA4:
        and_n8(regs->h, regs);
        return 4;

    // AND A,L
    case 0xA5:
        and_n8(regs->l, regs);
        return 4;

    // AND A,(HL)
    case 0xA6:
        and_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // AND A,A
    case 0xA7:
        and_n8(regs->a, regs);
        return 4;

    // XOR A,B
    case 0xA8:
        xor_n8(regs->b, regs);
        return 4;

    // XOR A,C
    case 0xA9:
        xor_n8(regs->c, regs);
        return 4;

    // XOR A,D
    case 0xAA:
        xor_n8(regs->d, regs);
        return 4;

    // XOR A,E
    case 0xAB:
        xor_n8(regs->e, regs);
        return 4;

    // XOR A,H
    case 0xAC:
        xor_n8(regs->h, regs);
        return 4;

    // XOR A,L
    case 0xAD:
        xor_n8(regs->l, regs);
        return 4;

    // XOR A,(HL)
    case 0xAE:
        xor_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // XOR A,A
    case 0xAF:
        xor_n8(regs->a, regs);
        return 4;

    // 0xB- ====================================================================
    // OR A,B
    case 0xB0:
        or_n8(regs->b, regs);
        return 4;

    // OR A,C
    case 0xB1:
        or_n8(regs->c, regs);
        return 4;

    // OR A,D
    case 0xB2:
        or_n8(regs->d, regs);
        return 4;

    // OR A,E
    case 0xB3:
        or_n8(regs->e, regs);
        return 4;

    // OR A,H
    case 0xB4:
        or_n8(regs->h, regs);
        return 4;

    // OR A,L
    case 0xB5:
        or_n8(regs->l, regs);
        return 4;

    // OR A,(HL)
    case 0xB6:
        or_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // OR A,A
    case 0xB7:
        or_n8(regs->a, regs);
        return 4;

    // CP A,B
    case 0xB8:
        cp_n8(regs->b, regs);
        return 4;

    // CP A,C
    case 0xB9:
        cp_n8(regs->c, regs);
        return 4;

    // CP A,D
    case 0xBA:
        cp_n8(regs->d, regs);
        return 4;

    // CP A,E
    case 0xBB:
        cp_n8(regs->e, regs);
        return 4;

    // cp A,H
    case 0xBC:
        cp_n8(regs->h, regs);
        return 4;

    // cp A,L
    case 0xBD:
        cp_n8(regs->l, regs);
        return 4;

    // CP A,(HL)
    case 0xBE:
        cp_n8(cpu_read8(cpu, regs->hl), regs);
        return 8;

    // CP A,A
    case 0xBF:
        cp_n8(regs->a, regs);
        return 4;

    // 0xC- ====================================================================
    // POP BC
    case 0xC1:
        pop(&regs->bc, cpu);
        return 12;

    // JP n16
    case 0xC3:
        jp(next_token16(cpu), cpu);
        return 16;

    // PUSH BC
    case 0xC5:
        push(regs->bc, cpu);
        return 16;

    // ADD A,u8
    case 0xC6:
        add_n8(next_token8(cpu), regs);
        return 8;

    // RET
    case 0xC9:
        ret(cpu);
        return 16;

    // PREFIX CB
    case 0xCB:
        return cpu_step_cb(next_token8(cpu), cpu);

    // ADC A,u8
    case 0xCE:
        adc_n8(next_token8(cpu), regs);
        return 8;

    // CALL u16
    case 0xCD:
        call(next_token16(cpu), cpu);
        return 24;

    // 0xD- ====================================================================
    // POP DE
    case 0xD1:
        pop(&regs->de, cpu);
        return 12;

    // PUSH DE
    case 0xD5:
        push(regs->de, cpu);
        return 16;

    // SUB A,u8
    case 0xD6:
        sub_n8(next_token8(cpu), regs);
        return 8;

    // RETI
    case 0xD9:
        reti(cpu);
        return 16;

    // SBC A,u8
    case 0xDE:
        sbc_n8(next_token8(cpu), regs);
        return 8;

    // 0xE- ====================================================================
    // LDH (n8),A
    case 0xE0:
        ld_m_n8(0xFF00 + next_token8(cpu), regs->a, cpu);
        return 12;

    // POP HL
    case 0xE1:
        pop(&regs->hl, cpu);
        return 12;

    // LDH (c),A
    case 0xE2:
        ld_m_n8(0xFF00 + regs->c, regs->a, cpu);
        return 8;

    // PUSH HL
    case 0xE5:
        push(regs->hl, cpu);
        return 16;

    // AND A,u8
    case 0xE6:
        and_n8(next_token8(cpu), regs);
        return 8;

    // ADD SP,u8
    case 0xE8:
        add_sp((int8_t)next_token8(cpu), cpu);
        return 16;

    // JP HL
    case 0xE9:
        regs->pc = regs->hl; // no cycles
        return 4;

    // LD (u16),A
    case 0xEA:
        ld_m_n8(next_token16(cpu), regs->a, cpu);
        return 16;

    // XOR A,u8
    case 0xEE:
        xor_n8(next_token8(cpu), regs);
        return 8;

    // 0xF- ====================================================================
    // LDH a,(n8)
    case 0xF0:
        ld_r8_n8(&regs->a, cpu_read8(cpu, 0xFF00 + next_token8(cpu)));
        return 12;

    // POP AF
    case 0xF1:
        pop(&regs->af, cpu);
        regs->f &= 0xF0;
        return 12;

    // LDH A,(c)
    case 0xF2:
        ld_r8_n8(&regs->a, cpu_read8(cpu, 0xFF00 + regs->c));
        return 8;

    // DI
    case 0xF3:
        di(regs);
        return 4;

    // PUSH AF
    case 0xF5:
        push(regs->af, cpu);
        return 16;

    // OR A,u8
    case 0xF6:
        or_n8(next_token8(cpu), regs);
        return 8;

    // LD HL,SP+i8
    case 0xF8:
        ld_hl_spe((int8_t)next_token8(cpu), cpu);
        return 12;

    // LD SP,HL
    case 0xF9:
        ld_r16_n16(&regs->sp, regs->hl);
        cpu_idle(cpu);
        return 8;

    // LD A,(u16)
    case 0xFA:
        ld_r8_n8(&regs->a, cpu_read8(cpu, next_token16(cpu)));
        return 16;

    // EI
    case 0xFB:
        ei(regs);
        return 4;

    // CP A,n8
    case 0xFE:
        cp_n8(next_token8(cpu), regs);
        return 8;

    // -------------------------------------------------------------------------
    // JUMPS
    // JR cc,i8
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38: {
        const uint8_t operand = next_token8(cpu);
        if (cond_met(opcode, regs)) {
            jr((int8_t)operand, cpu);
            return 12;
        }
        return 8;
    }
    // JP cc,a16
    case 0xC2:
    case 0xCA:
    case 0xD2:
    case 0xDA: {
        const uint16_t operand = next_token16(cpu);
        if (cond_met(opcode, regs)) {
            jp(operand, cpu);
            return 16;
        }
        return 12;
    }

    // CALL cc/RET cc/RST
    case 0xC4:
    case 0xCC:
    case 0xD4:
    case 0xDC: {
        const uint16_t operand = next_token16(cpu);
        if (cond_met(opcode, regs)) {
            call(operand, cpu);
            return 24;
        }
        return 12;
    }
    case 0xC0:
    case 0xC8:
    case 0xD0:
    case 0xD8:
        cpu_idle(cpu);
        if (cond_met(opcode, regs)) {
            ret(cpu);
            return 20;
        }
        return 8;
    case 0xC7:
    case 0xCF:
    case 0xD7:
    case 0xDF:
    case 0xE7:
    case 0xEF:
    case 0xF7:
    case 0xFF:
        call(opcode & 0x38, cpu);
        return 16;

    default:
        fprintf(stderr, "error opcode: %02X\n", opcode);
        fprintf(stderr, "error pc    : %04X\n", (unsigned int)(regs->pc - 1));
        exit(1);
    }
    
}

void cpu_step(struct cpu *cpu, bool trace) {
    const uint64_t before = cpu->cycles;
    const uint8_t declared = cpu_execute(cpu, trace);
    assert(cpu->cycles - before == declared);
    while (cpu->cycles - before < declared)
        cpu_tick(cpu);
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
