#ifndef GB_H_
#define GB_H_

#include "cpu.h"
#include "bus.h"
#include "ppu.h"
#include "timer.h"
#include <stdbool.h>

struct gb {
    struct cpu_regs regs;
    struct bus bus;
    struct ppu ppu;
    struct timer timer;
    struct cart cart;

    uint64_t cycles; // t-cycles
    bool trace;
};

void gb_init(struct gb *gb, const char *rom_path, const bool trace);
void gb_step(struct gb *gb);

#endif // GB_H_
