#include "gb.h"
#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>

void gb_init(struct gb *gb, const char *rom_path, const bool trace) {
    gb->trace = trace;
    gb->cycles = 0;
    if (cart_load(&gb->cart, rom_path)) {
        fprintf(stderr, "Error: invalid cart %s", rom_path);
        exit(1);
    }
    cart_print(&gb->cart);
    bus_mem_init(&gb->bus, &gb->cart, &gb->ppu);
    cpu_init(&gb->regs);
    ppu_init(&gb->ppu);
    cpu_skip_boot(&gb->regs, &gb->bus);
}

void gb_step(struct gb *gb) {
    const size_t cycles = cpu_step(&gb->regs, &gb->bus, gb->trace);
    ppu_tick(&gb->ppu, cycles, &gb->bus);
    gb->cycles += cycles;
}
