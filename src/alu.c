#include "alu.h"

// add
void alu_add_r8(uint8_t val, uint8_t carry_in, struct cpu_regs *regs) {
    uint16_t res = regs->a + val + carry_in;
    regs->f_bits.c = res > 0xFF;
    regs->f_bits.h = ((regs->a & 0x0F) + (val & 0x0F) + carry_in) > 0x0F;
    regs->a = res & 0xFF;
    regs->f_bits.z = !(regs->a);
    regs->f_bits.n = 0;
}
void alu_add_r16(uint16_t *dst, uint16_t val, struct cpu_regs *regs) {
    uint32_t res = *dst + val;
    regs->f_bits.n = 0;
    regs->f_bits.h = ((*dst & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF;
    regs->f_bits.c = res > 0xFFFF;
    *dst = res & 0xFFFF;
}

// sub
void alu_sub_r8(uint8_t val, uint8_t carry_in, struct cpu_regs *regs) {
    uint16_t res = regs->a - val - carry_in;
    regs->f_bits.c = regs->a < (val + carry_in);
    regs->f_bits.h = (regs->a & 0x0F) < ((val & 0x0F) + carry_in);
    regs->a = res & 0xFF;
    regs->f_bits.z = !(regs->a);
    regs->f_bits.n = 1;
}

// rotates/swap
void alu_rlc(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = (*dst >> 7) & 0x1;
    *dst = (*dst << 1) | (*dst >> 7);
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}

void alu_rrc(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = *dst & 0x1;
    *dst = (*dst << 7) | (*dst >> 1);
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}
void alu_rl(uint8_t *dst, struct cpu_regs *regs) {
    const uint8_t res = (*dst << 1) | regs->f_bits.c;
    regs->f_bits.c = (*dst >> 7) & 0x1;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (res == 0);
    *dst = res;
}
void alu_rr(uint8_t *dst, struct cpu_regs *regs) {
    const uint8_t res = (regs->f_bits.c << 7) | (*dst >> 1);
    regs->f_bits.c = *dst & 0x1;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (res == 0);
    *dst = res;
}
void alu_sla(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = (*dst >> 7) & 0x1;
    *dst = *dst << 1;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}
void alu_sra(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = *dst & 0x1;
    *dst = (*dst >> 1) | (*dst & 0x80);
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}
void alu_srl(uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.c = *dst & 0x1;
    *dst = *dst >> 1;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.z = (*dst == 0);
}
void alu_swap(uint8_t *dst, struct cpu_regs *regs) {
    *dst = ((*dst & 0x0F) << 4) | ((*dst & 0xF0) >> 4);
    regs->f_bits.c = 0;
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


// logical
void alu_and(uint8_t val, struct cpu_regs *regs) {
    regs->a = regs->a & val;
    regs->f_bits.z = !regs->a;
    regs->f_bits.n = 0;
    regs->f_bits.h = 1;
    regs->f_bits.c = 0;
}
void alu_or(uint8_t val, struct cpu_regs *regs) {
    regs->a = regs->a | val;
    regs->f_bits.z = !regs->a;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.c = 0;
}
void alu_xor(uint8_t val, struct cpu_regs *regs) {
    regs->a = regs->a ^ val;
    regs->f_bits.z = !regs->a;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
    regs->f_bits.c = 0;
}

void alu_cpl(struct cpu_regs *regs) {
    regs->a = ~regs->a;
    regs->f_bits.n = 1;
    regs->f_bits.h = 1;
}

void alu_cp(uint8_t val, struct cpu_regs *regs) {
    regs->f_bits.z = !(regs->a - val);
    regs->f_bits.n = 1;
    regs->f_bits.h = (regs->a & 0x0F) < (val & 0x0F);
    regs->f_bits.c = regs->a < val;
}

void alu_daa(struct cpu_regs *regs) {
    uint8_t adj = 0;

    if (regs->f_bits.n) {
        if (regs->f_bits.h) adj += 0x6;
        if (regs->f_bits.c) adj += 0x60;
        regs->a -= adj;
    } else {
        if (regs->f_bits.h || ((regs->a & 0xF) > 0x9)) adj += 0x6;
        if (regs->f_bits.c || (regs->a > 0x99)) {
            adj += 0x60;
            regs->f_bits.c = 1;
        }
        regs->a += adj;
    }

    regs->f_bits.z = !regs->a;
    regs->f_bits.h = 0;
}

void alu_scf(struct cpu_regs *regs) {
    regs->f_bits.c = 1;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
}

void alu_ccf(struct cpu_regs *regs) {
    regs->f_bits.c = !regs->f_bits.c;
    regs->f_bits.n = 0;
    regs->f_bits.h = 0;
}

// bit/res/set
void alu_bit(uint8_t idx, uint8_t *dst, struct cpu_regs *regs) {
    regs->f_bits.z = !(*dst & (1 << idx));
    regs->f_bits.n = 0;
    regs->f_bits.h = 1;
}
void alu_res(uint8_t idx, uint8_t *dst) { *dst = (*dst & ~(1 << idx)); }
void alu_set(uint8_t idx, uint8_t *dst) { *dst = (*dst | (1 << idx)); }
