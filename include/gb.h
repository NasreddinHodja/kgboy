#ifndef GB_H_
#define GB_H_

#include "cpu.h"
#include "bus.h"
#include "ppu.h"
#include "timer.h"
#include "joypad.h"
#include <stdbool.h>

struct gb {
    struct cpu cpu;
    struct bus bus;
    struct ppu ppu;
    struct timer timer;
    struct joypad jp;
    struct cart cart;

    bool trace;
};

bool gb_init(struct gb *gb, const char *rom_path, const bool trace);
void gb_destroy(struct gb *gb);
void gb_step(struct gb *gb);

#endif // GB_H_
