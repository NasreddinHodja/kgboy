#ifndef OPS_H_
#define OPS_H_

#include "cpu.h"
#include <stdint.h>

// ** UNPREFIXED

void nop();
void stop();
void halt(struct cpu_regs *regs);

// interrupts
void ei(struct cpu_regs *regs);
void di(struct cpu_regs *regs);
void reti(struct cpu *cpu);

// ld
void ld_r8_n8(uint8_t *dst, uint8_t val);
void ld_r16_n16(uint16_t *dst, uint16_t val);
void ld_m_n8(uint16_t addr, uint8_t val, struct cpu *cpu);
void ld_m_n16(uint16_t addr, uint16_t val, struct cpu *cpu);
void ld_hl_spe(int8_t off, struct cpu *cpu);

// inc/dec
void inc_r8(uint8_t *dst, struct cpu_regs *regs);
void inc_r16(uint16_t *dst, struct cpu *cpu);
void inc_m(uint16_t addr, struct cpu *cpu);
void dec_r8(uint8_t *dst, struct cpu_regs *regs);
void dec_r16(uint16_t *dst, struct cpu *cpu);
void dec_m(uint16_t addr, struct cpu *cpu);

// rotates
void rlca(struct cpu_regs *regs);
void rla(struct cpu_regs *regs);
void rra(struct cpu_regs *regs);

// rrca
void rrca(struct cpu_regs *regs);

// add
void add_r16_n16(uint16_t *dst, uint16_t val, struct cpu *cpu);
void add_n8(uint8_t val, struct cpu_regs *regs);
void adc_n8(uint8_t val, struct cpu_regs *regs);
void add_sp(int8_t val, struct cpu *cpu);

// sub
void sub_n8(uint8_t val, struct cpu_regs *regs);
void sbc_n8(uint8_t val, struct cpu_regs *regs);

// logical
void or_n8(uint8_t val, struct cpu_regs *regs);
void and_n8(uint8_t val, struct cpu_regs *regs);
void xor_n8(uint8_t val, struct cpu_regs *regs);
void cpl(struct cpu_regs *regs);

// call/ret/push/pop
void push(uint16_t val, struct cpu *cpu);
void pop(uint16_t *dst, struct cpu *cpu);
void call(uint16_t addr, struct cpu *cpu);
void ret(struct cpu *cpu);

// jr
void jr(int8_t off, struct cpu *cpu);
void jp(uint16_t addr, struct cpu *cpu);

// cp
void cp_n8(uint8_t val, struct cpu_regs *regs);

// daa
void daa(struct cpu_regs *regs);

// scf
void scf(struct cpu_regs *regs);

// ccf
void ccf(struct cpu_regs *regs);

// 0xCB PREXIFED
uint8_t cpu_step_cb(uint8_t opcode, struct cpu *cpu);

#endif // OPS_H_

