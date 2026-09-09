#include "gb.h"
#include <stdio.h>

bool gb_init(struct gb *gb, const char *rom_path, const bool trace) {
    gb->trace = trace;

    // load cart
    if (cart_load(&gb->cart, rom_path)) {
        fprintf(stderr, "Error: invalid cart %s", rom_path);
        return false;
    }
    cart_print(&gb->cart);

    timer_init(&gb->timer);

    joypad_init(&gb->jp);

    // bus
    bus_mem_init(&gb->bus, &gb->cart, &gb->ppu, &gb->timer, &gb->jp);
    ppu_init(&gb->ppu);
    cpu_init(&gb->cpu, &gb->bus, &gb->ppu, &gb->timer);
    cpu_skip_boot(&gb->cpu);

    return true;
}

void gb_destroy(struct gb *gb) {
    cart_free(&gb->cart);
}

void gb_step(struct gb *gb) {
    cpu_step(&gb->cpu, gb->trace);
}

