#include "ops.h"
#include "alu.h"
#include "cpu.h"

void nop(void) { return; }

void stop(void) {
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
void reti(struct cpu *cpu) {
    cpu->regs.ime = true;
    cpu->regs.ime_pending = false;
    ret(cpu);
}

// load reg
void ld_r8_n8(uint8_t *dst, uint8_t val) { *dst = val; }
void ld_r16_n16(uint16_t *dst, uint16_t val) { *dst = val; }

// load mem
void ld_m_n8(uint16_t addr, uint8_t val, struct cpu *cpu) {
    cpu_write8(cpu, addr, val);
}
void ld_m_n16(uint16_t addr, uint16_t val, struct cpu *cpu) {
    cpu_write8(cpu, addr, val);
    cpu_write8(cpu, addr + 1, val >> 8);
}

// ld hl,sp+i8
void ld_hl_spe(int8_t off, struct cpu *cpu) {
    ld_r16_n16(&cpu->regs.hl, cpu->regs.sp + off);
    cpu->regs.f_bits.h = ((cpu->regs.sp & 0x0F) + (off & 0x0F)) > 0x0F;
    cpu->regs.f_bits.c = ((cpu->regs.sp & 0xFF) + (off & 0xFF)) > 0xFF;
    cpu->regs.f_bits.z = 0;
    cpu->regs.f_bits.n = 0;
    cpu_idle(cpu);
}

// increment/decrement
void inc_r16(uint16_t *dst, struct cpu *cpu) {
    alu_inc_r16(dst);
    cpu_idle(cpu);
}
void inc_m(uint16_t addr, struct cpu *cpu) {
    uint8_t t = cpu_read8(cpu, addr);
    alu_inc_r8(&t, &cpu->regs);
    cpu_write8(cpu, addr, t);
}

void dec_r16(uint16_t *dst, struct cpu *cpu) {
    alu_dec_r16(dst);
    cpu_idle(cpu);
}
void dec_m(uint16_t addr, struct cpu *cpu) {
    uint8_t t = cpu_read8(cpu, addr);
    alu_dec_r8(&t, &cpu->regs);
    cpu_write8(cpu, addr, t);
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
void add_r16_n16(uint16_t *dst, uint16_t val, struct cpu *cpu) {
    alu_add_r16(dst, val, &cpu->regs);
    cpu_idle(cpu);
}

void add_sp(int8_t val, struct cpu *cpu) {
    cpu->regs.f_bits.h = ((cpu->regs.sp & 0x0F) + (val & 0x0F)) > 0x0F;
    cpu->regs.f_bits.c = ((cpu->regs.sp & 0xFF) + (val & 0xFF)) > 0xFF;
    cpu->regs.f_bits.n = 0;
    cpu->regs.f_bits.z = 0;
    cpu->regs.sp += val;
    cpu_idle(cpu);
    cpu_idle(cpu);
}

// call/ret/push/pop
void push(uint16_t val, struct cpu *cpu) {
    cpu_idle(cpu);
    cpu_write8(cpu, --cpu->regs.sp, val >> 8);
    cpu_write8(cpu, --cpu->regs.sp, val);
}

void pop(uint16_t *dst, struct cpu *cpu) {
    const uint8_t lo = cpu_read8(cpu, cpu->regs.sp++);
    const uint8_t hi = cpu_read8(cpu, cpu->regs.sp++);
    *dst = lo | hi << 8;
}

void call(uint16_t addr, struct cpu *cpu) {
    push(cpu->regs.pc, cpu);
    cpu->regs.pc = addr;
}

void ret(struct cpu *cpu) {
    pop(&cpu->regs.pc, cpu);
    cpu_idle(cpu);
}

// jumps
void jr(int8_t off, struct cpu *cpu) {
    cpu->regs.pc += off;
    cpu_idle(cpu);
}

void jp(uint16_t addr, struct cpu *cpu) {
    cpu->regs.pc = addr;
    cpu_idle(cpu);
}

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
        return NULL; // (HL) - caller reads/writes memory
    case 7:
        return &regs->a;
    }
    return NULL; // unreachable with the & 7
}

uint8_t cpu_step_cb(uint8_t opcode, struct cpu *cpu) {
    static void (*const cb_ops[8])(uint8_t *, struct cpu_regs *) = {
        alu_rlc, alu_rrc, alu_rl, alu_rr, alu_sla, alu_sra, alu_swap, alu_srl,
    };

    const uint8_t group = opcode >> 6;       // 0:shift/rot 1s;BIT 2:RES 3:SET
    const uint8_t sel = (opcode >> 3) & 0x7; // op selector / bit index
    const uint8_t idx = opcode & 0x7;        // B C D E H L (HL) A

    uint8_t val = 0;
    uint8_t *dst = reg_operand(idx, &cpu->regs);
    const bool is_mem = (dst == NULL);

    if (is_mem) {
        val = cpu_read8(cpu, cpu->regs.hl);
        dst = &val;
    }

    switch (group) {
    case 0:
        cb_ops[sel](dst, &cpu->regs);
        break;
    case 1:
        alu_bit(sel, dst, &cpu->regs);
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
        cpu_write8(cpu, cpu->regs.hl, val);

    if (!is_mem)
        return 8;
    return (group == 1) ? 12 : 16;
}
