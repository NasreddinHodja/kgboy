#include "gb.h"
#include "ppu.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>

void gb_init(struct gb *gb, const char *rom_path, const bool trace) {
    gb->trace = trace;
    gb->cycles = 0;

    // load cart
    if (cart_load(&gb->cart, rom_path)) {
        fprintf(stderr, "Error: invalid cart %s", rom_path);
        exit(1);
    }
    cart_print(&gb->cart);

    // timer
    timer_init(&gb->timer);
    bus_mem_init(&gb->bus, &gb->cart, &gb->ppu, &gb->timer);
    cpu_init(&gb->regs);
    ppu_init(&gb->ppu);
    cpu_skip_boot(&gb->regs, &gb->bus);
}

void gb_step(struct gb *gb) {
    const size_t cycles = cpu_step(&gb->regs, &gb->bus, gb->trace);
    timer_tick(&gb->timer, cycles, &gb->bus);
    ppu_tick(&gb->ppu, cycles, &gb->bus);
    gb->cycles += cycles;
}
