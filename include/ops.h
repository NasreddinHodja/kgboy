#ifndef OPS_H_
#define OPS_H_

#include "cpu.h"
#include "bus.h"
#include <stdint.h>

// UNPREFIXED

void nop();
void stop();

// LD
void ld_r8_n8(uint8_t *dst, uint8_t val);
void ld_r16_n16(uint16_t *dst, uint16_t val);
void ld_m_n8(uint16_t addr, uint8_t val, struct bus *bus);
void ld_m_n16(uint16_t addr, uint16_t val, struct bus *bus);

// INC
void inc_r8(uint8_t *dst, struct cpu_regs *regs);
void inc_r16(uint16_t *dst);

// DEC
void dec_r8(uint8_t *dst, struct cpu_regs *regs);
void dec_r16(uint16_t *dst);

// RLCA
void rlca(struct cpu_regs *regs);

// RRCA
void rrca(struct cpu_regs *regs);

// ADD
void add_r16_n16(uint16_t *dst, uint16_t val, struct cpu_regs *regs);

// OR
void or_n8(uint8_t val, struct cpu_regs *regs);

// JR
void jr_e8(int8_t off, struct cpu_regs *regs);

// CP
void cp_n8(int8_t val, struct cpu_regs *regs);

#endif // OPS_H_

