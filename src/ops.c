#include "ops.h"
#include "alu.h"
#include "cpu.h"
#include "bus.h"

void nop() { return; }

void stop() {
    // https://gbdev.io/pandocs/Reducing_Power_Consumption.html#using-the-stop-instruction
    // lol
    return;
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

// increment
void inc_r8(uint8_t *dst, struct cpu_regs *regs) { alu_inc_r8(dst, regs); }
void inc_r16(uint16_t *dst) { alu_inc_r16(dst); }

// rlc A
void rlca(struct cpu_regs *regs) {
    alu_rlc(&regs->a, regs);
    regs->f_bits.z = 0;
}

// rrc A
void rrca(struct cpu_regs *regs) {
    alu_rrc(&regs->a, regs);
    regs->f_bits.z = 0;
}

// add u16, u16
void add_r16_n16(uint16_t *dst, uint16_t val, struct cpu_regs *regs) {
    alu_add_r16(dst, val, regs);
}

// decrease
void dec_r8(uint8_t *dst, struct cpu_regs *regs) { alu_dec_r8(dst, regs); }
void dec_r16(uint16_t *dst) { alu_dec_r16(dst); }

// or
void or_n8(uint8_t val, struct cpu_regs *regs) { alu_or(val, regs); }

// jr
void jr_e8(int8_t off, struct cpu_regs *regs) { regs->pc += off; }

// cp
void cp_n8(int8_t val, struct cpu_regs *regs) {
    alu_cp(val, regs);
}
