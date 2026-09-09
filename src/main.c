#include "bus.h"
#include "cpu.h"
#include "display.h"
#include "gb.h"
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
    if (!gb_init(&gb, rom_path, trace)) {
        return 1;
    }

    struct display disp;
    if (!display_init(&disp)) {
        gb_destroy(&gb);
        return 1;
    }

    uint32_t steps = 0;
    bool running = true;
    while (running) {
        if (trace)
            fprintf(stdout, "Step %u\n", steps++);

        gb_step(&gb);

        if (gb.ppu.frame_ready) {
            gb.ppu.frame_ready = false;
            display_present(&disp, gb.ppu.fb);
            running = display_poll(&gb.jp);
        }

        if (trace)
            fprintf(stdout, "Ran %lu t-cycles.\n\n", gb.cpu.cycles);
    }

    gb_destroy(&gb);
    display_destroy(&disp);

    return 0;
}
