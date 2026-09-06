#include "cpu.h"
#include "display.h"
#include "gb.h"
#include "bus.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // parse args
    char *rom_path = NULL;
    bool trace = false;
    int opt;

    while ((opt = getopt(argc, argv, "r:t")) != -1) {
        switch (opt) {
        case 'r':
            rom_path = optarg;
            break;
        case 't':
            trace = true;
            break;
        case '?':
            return 1;
        default:
            break;
        }
    }

    if (!rom_path) {
        fprintf(stderr, "usage: %s -r <rom_path> [-t]\n", argv[0]);
        return 1;
    }

    struct gb gb;
    gb_init(&gb, rom_path, trace);

    struct display disp;
    display_init(&disp);

    fprintf(stdout, "\n");

    uint32_t steps = 0;
    bool running = true;
    while (running) {
        if (trace) fprintf(stdout, "Step %u\n", steps);
        gb_step(&gb);
        if (gb.ppu.frame_ready) {
            gb.ppu.frame_ready = false;
            display_present(&disp, gb.ppu.fb, gb.ppu.bgp);
            running = display_poll();
        }
        if (trace) fprintf(stdout, "Ran %lu t-cycles.\n\n", gb.cycles);
    }

    // step
    fprintf(stdout, "\n");

    // print state
    cpu_regs_print(&gb.regs);
    fprintf(stdout, "\n");

    fprintf(stdout, "Tile Data block 0 - $8000:\n");
    bus_mem_print(&gb.bus, 0x8000, 64);
    fprintf(stdout, "\n");
    fprintf(stdout, "Tile Map 0 - $9800:\n");
    bus_mem_print(&gb.bus, 0x9800, 128);
    fprintf(stdout, "\n");

    return 0;
}
