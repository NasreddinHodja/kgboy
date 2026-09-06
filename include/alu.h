#ifndef ALU_H_
#define ALU_H_

#include "cpu.h"
#include <stdint.h>

// +/-
void alu_add_r8(uint8_t val, uint8_t carry_in, struct cpu_regs *regs);
void alu_sub_r8(uint8_t val, uint8_t carry_in, struct cpu_regs *regs);
void alu_add_r16(uint16_t *dst, uint16_t val, struct cpu_regs *regs);

// logical
void alu_and(uint8_t val, struct cpu_regs *regs);
void alu_or(uint8_t val, struct cpu_regs *regs);
void alu_xor(uint8_t val, struct cpu_regs *regs);
void alu_cpl(struct cpu_regs *regs);

// compare
void alu_cp(uint8_t val, struct cpu_regs *regs);

// increments
void alu_inc_r8(uint8_t *dst, struct cpu_regs *regs);
void alu_inc_r16(uint16_t *dst);
void alu_dec_r8(uint8_t *dst, struct cpu_regs *regs);
void alu_dec_r16(uint16_t *dst);

// rotates/shifts/swaps
void alu_rl(uint8_t *dst, struct cpu_regs *regs);
void alu_rlc(uint8_t *dst, struct cpu_regs *regs);
void alu_rr(uint8_t *dst, struct cpu_regs *regs);
void alu_rrc(uint8_t *dst, struct cpu_regs *regs);
void alu_sla(uint8_t *dst, struct cpu_regs *regs);
void alu_sra(uint8_t *dst, struct cpu_regs *regs);
void alu_srl(uint8_t *dst, struct cpu_regs *regs);
void alu_swap(uint8_t *dst, struct cpu_regs *regs);

// bit/res/set
void alu_bit(uint8_t idx, uint8_t *dst, struct cpu_regs *regs);
void alu_res(uint8_t idx, uint8_t *dst);
void alu_set(uint8_t idx, uint8_t *dst);

// daa
void alu_daa(struct cpu_regs *regs);

// scf
void alu_scf(struct cpu_regs *regs);

// ccf
void alu_ccf(struct cpu_regs *regs);

#endif // ALU_H_
