#include "ops.h"
#include "alu.h"
#include "bus.h"
#include "cpu.h"

void nop() { return; }

void stop() {
    // TODO:
    // https://gbdev.io/pandocs/Reducing_Power_Consumption.html#using-the-stop-instruction
    // lol
    return;
}

void halt(struct cpu_regs *regs) { regs->halted = true; }

// interrupts
void ei(struct cpu_regs *regs) { regs->ime_pending = true; }
void di(struct cpu_regs *regs) {
    regs->ime = false;
    regs->ime_pending = false;
}
void reti(struct cpu_regs *regs, struct bus *bus) {
    regs->ime = true;
    regs->ime_pending = false;
    ret(regs, bus);
}

// load reg
void ld_r8_n8(uint8_t *dst, uint8_t val) { *dst = val; }
void ld_r16_n16(uint16_t *dst, uint16_t val) { *dst = val; }

// load mem
void ld_m_n8(uint16_t addr, uint8_t val, struct bus *bus) {
    bus_mem_write8(bus, addr, val);
}
void ld_m_n16(uint16_t addr, uint16_t val, struct bus *bus) {
    bus_mem_write16(bus, addr, val);
}

// ld hl,sp+i8
void ld_hl_spe(int8_t off, struct cpu_regs *regs) {
    ld_r16_n16(&regs->hl, regs->sp + off);
    regs->f_bits.h = ((regs->sp & 0x0F) + (off & 0x0F)) > 0x0F;
    regs->f_bits.c = ((regs->sp & 0xFF) + (off & 0xFF)) > 0xFF;
    regs->f_bits.z = 0;
    regs->f_bits.n = 0;
}

// increment/decrement
void inc_r8(uint8_t *dst, struct cpu_regs *regs) { alu_inc_r8(dst, regs); }
void inc_r16(uint16_t *dst) { alu_inc_r16(dst); }
void inc_m(uint16_t addr, struct cpu_regs *regs, struct bus *bus) {
    uint8_t t = bus_mem_read8(bus, addr);
    alu_inc_r8(&t, regs);
    bus_mem_write8(bus, addr, t);
}

void dec_r8(uint8_t *dst, struct cpu_regs *regs) { alu_dec_r8(dst, regs); }
void dec_r16(uint16_t *dst) { alu_dec_r16(dst); }
void dec_m(uint16_t addr, struct cpu_regs *regs, struct bus *bus) {
    uint8_t t = bus_mem_read8(bus, addr);
    alu_dec_r8(&t, regs);
    bus_mem_write8(bus, addr, t);
}

// rotate
void rlca(struct cpu_regs *regs) {
    alu_rlc(&regs->a, regs);
    regs->f_bits.z = 0;
}

void rrca(struct cpu_regs *regs) {
    alu_rrc(&regs->a, regs);
    regs->f_bits.z = 0;
}

void rla(struct cpu_regs *regs) {
    alu_rl(&regs->a, regs);
    regs->f_bits.z = 0;
}

void rra(struct cpu_regs *regs) {
    alu_rr(&regs->a, regs);
    regs->f_bits.z = 0;
}

// add
void add_r16_n16(uint16_t *dst, uint16_t val, struct cpu_regs *regs) {
    alu_add_r16(dst, val, regs);
}
void add_n8(uint8_t val, struct cpu_regs *regs) { alu_add_r8(val, 0, regs); }
void adc_n8(uint8_t val, struct cpu_regs *regs) {
    alu_add_r8(val, regs->f_bits.c, regs);
}
void add_sp(int8_t val, struct cpu_regs *regs) {
    regs->f_bits.h = ((regs->sp & 0x0F) + (val & 0x0F)) > 0x0F;
    regs->f_bits.c = ((regs->sp & 0xFF) + (val & 0xFF)) > 0xFF;
    regs->f_bits.n = 0;
    regs->f_bits.z = 0;
    regs->sp += val;
}

// sub
void sub_n8(uint8_t val, struct cpu_regs *regs) { alu_sub_r8(val, 0, regs); }
void sbc_n8(uint8_t val, struct cpu_regs *regs) {
    alu_sub_r8(val, regs->f_bits.c, regs);
}

// logical
void or_n8(uint8_t val, struct cpu_regs *regs) { alu_or(val, regs); }
void and_n8(uint8_t val, struct cpu_regs *regs) { alu_and(val, regs); }
void xor_n8(uint8_t val, struct cpu_regs *regs) { alu_xor(val, regs); }
void cp_n8(uint8_t val, struct cpu_regs *regs) { alu_cp(val, regs); }
void cpl(struct cpu_regs *regs) { alu_cpl(regs); }

// call/ret/push/pop
void push(uint16_t val, struct cpu_regs *regs, struct bus *bus) {
    regs->sp -= 2;
    bus_mem_write16(bus, regs->sp, val);
}
void pop(uint16_t *dst, struct cpu_regs *regs, struct bus *bus) {
    *dst = bus_mem_read16(bus, regs->sp);
    regs->sp += 2;
}
void call(uint16_t addr, struct cpu_regs *regs, struct bus *bus) {
    push(regs->pc, regs, bus);
    jp(addr, regs);
}
void ret(struct cpu_regs *regs, struct bus *bus) { pop(&regs->pc, regs, bus); }

// jumps
void jr(int8_t off, struct cpu_regs *regs) { regs->pc += off; }
void jp(uint16_t addr, struct cpu_regs *regs) { regs->pc = addr; }

// daa
void daa(struct cpu_regs *regs) { alu_daa(regs); }

// scf
void scf(struct cpu_regs *regs) { alu_scf(regs); }

// ccf
void ccf(struct cpu_regs *regs) { alu_ccf(regs); }

// PREFIXED
static uint8_t *reg_operand(uint8_t idx, struct cpu_regs *regs) {
    switch (idx & 7) {
    case 0:
        return &regs->b;
    case 1:
        return &regs->c;
    case 2:
        return &regs->d;
    case 3:
        return &regs->e;
    case 4:
        return &regs->h;
    case 5:
        return &regs->l;
    case 6:
        return NULL; // (HL) -- caller reads/writes memory
    case 7:
        return &regs->a;
    }
    return NULL; // unreachable with the & 7
}

uint8_t cpu_step_cb(uint8_t opcode, struct cpu_regs *regs, struct bus *bus) {
    static void (*const cb_ops[8])(uint8_t *, struct cpu_regs *) = {
        alu_rlc, alu_rrc, alu_rl, alu_rr, alu_sla, alu_sra, alu_swap, alu_srl,
    };

    const uint8_t group = opcode >> 6;       // 0:shift/rot 1s;BIT 2:RES 3:SET
    const uint8_t sel = (opcode >> 3) & 0x7; // op selector / bit index
    const uint8_t idx = opcode & 0x7;        // B C D E H L (HL) A

    uint8_t val = 0;
    uint8_t *dst = reg_operand(idx, regs);
    const bool is_mem = (dst == NULL);

    if (is_mem) {
        val = bus_mem_read8(bus, regs->hl);
        dst = &val;
    }

    switch (group) {
    case 0:
        cb_ops[sel](dst, regs);
        break;
    case 1:
        alu_bit(sel, dst, regs);
        break;
    case 2:
        alu_res(sel, dst);
        break;
    case 3:
        alu_set(sel, dst);
        break;
    }

    // BIT only reads; every other op writes back
    if (is_mem && group != 1)
        bus_mem_write8(bus, regs->hl, val);

    if (!is_mem)
        return 8;
    return (group == 1) ? 12 : 16;
}
