#include "alu.h"

// add 8
void alu_add_r8(uint8_t val, uint8_t carry_in, struct cpu_regs *regs) {
    uint16_t res = regs->a + val + carry_in;
    regs->f_bits.c = res > 0xFF;
    regs->f_bits.h = ((regs->a & 0x0F) + (val & 0x0F) + carry_in) > 0x0F;
    regs->a = res & 0xFF;
    regs->f_bits.z = !(regs->a);
    regs->f_bits.n = 0;
}
// add 16
void alu_add_r16(uint16_t *dst, uint16_t val, struct cpu_regs *regs) {
    uint32_t res = *dst + val;
    regs->f_bits.n = 0;
    regs->f_bits.h = ((*dst & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF;
    regs->f_bits.c = res > 0xFFFF;
    *dst = res & 0xFFFF;
}

// rotate left circular
void alu_rlc(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = (*dst >> 7) & 0x1;
    *dst = (*dst << 1) | (*dst >> 7);
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}

// rotate right circular
void alu_rrc(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = *dst & 0x1;
    *dst = (*dst << 7) | (*dst >> 1);
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}

// decrease
void alu_dec_r16(uint16_t *dst) { (*dst)--; }

void alu_dec_r8(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.h = ((*dst & 0xF) == 0);
    (*dst)--;
    regs->f_bits.z = !(*dst);
    regs->f_bits.n = 1;
}

// increase
void alu_inc_r8(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.h = (*dst & 0x0F) == 0x0F;
    (*dst)++;
    regs->f_bits.n = 0;
    regs->f_bits.z = !(*dst);
}
void alu_inc_r16(uint16_t *dst) { (*dst)++; }


// or
void alu_or(uint8_t val, struct cpu_regs *regs) {
    regs->a = regs->a | val;
    regs->f_bits.z = !regs->a;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.c = 0;
}

void alu_cp(uint8_t val, struct cpu_regs *regs) {
    regs->f_bits.z = !(regs->a - val);
    regs->f_bits.n = 1;
    regs->f_bits.h = (regs->a & 0x0F) < (val & 0x0F);
    regs->f_bits.c = regs->a < val;
}
