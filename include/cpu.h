#ifndef CPU_H_
#define CPU_H_

#include <stdbool.h>
#include <stdint.h>
#include "bus.h"

#define CLOCK_SPEED 4.194304 // MHz

struct cpu_regs {
    union {
        uint16_t af;
        struct {
            union {
                struct {
                    uint8_t unused : 4;
                    uint8_t c : 1;
                    uint8_t h : 1;
                    uint8_t n : 1;
                    uint8_t z : 1;
                } f_bits;
                uint8_t f;
            };
            uint8_t a;
        };
    };

    union {
        uint16_t bc;
        struct {
            uint8_t c;
            uint8_t b;
        };
    };

    union {
        uint16_t de;
        struct {
            uint8_t e;
            uint8_t d;
        };
    };

    union {
        uint16_t hl;
        struct {
            uint8_t l;
            uint8_t h;
        };
    };

    uint16_t sp;
    uint16_t pc;
};

void cpu_init(struct cpu_regs *regs);
void cpu_skip_boot(struct cpu_regs *regs, struct bus *bus);
uint8_t cpu_step(struct cpu_regs *regs, struct bus *bus, bool step);

void cpu_regs_print(struct cpu_regs *regs);
void cpu_step_print(struct cpu_regs *regs, struct bus *bus);

#endif // CPU_H_
