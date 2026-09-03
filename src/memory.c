#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void mem_init(struct memory *mem, struct cart *cart) {
    memset(mem, 0, sizeof(*mem));
    mem->cart = cart;
}

static uint8_t mem_read_high(struct memory *mem, uint16_t addr) {
    // still in echo RAM
    if (addr < 0xFE00)
        return mem->wram[addr & 0x1FFF];
    // OAM
    if (addr < 0xFEA0) { 
        return mem->oam[addr - 0xFE00];
    // NOT USABLE
    } else if (addr < 0xFF00) { 
        return 0xFF;
    // IO registers
    } else if (addr < 0xFF80) { 
        return mem->io[addr - 0xFF00];
    // HRAM
    } else if (addr < 0xFFFF) {
        return mem->hram[addr - 0xFF80];
    // IE register
    } else { 
        return mem->ie;
    }
    return 0xFF;
}

uint8_t mem_read(struct memory *mem, uint16_t addr) {
    switch (addr >> 12) {
        // ROM bank 00
        case 0x0: case 0x1: case 0x2: case 0x3: 
        // ROM bank NN
        case 0x4: case 0x5: case 0x6: case 0x7: 
            return mem->cart->rom[addr];
        // VRAM
        case 0x8: case 0x9:
            return mem->vram[addr & 0x1FFF];
        // TODO: E(xternal)RAM
        case 0xA: case 0xB:
            return 0xFF;
        // WRAM
        case 0xC: case 0xD:
            return mem->wram[addr & 0x1FFF];
        // Echo RAM
        case 0xE: 
            return mem->wram[addr & 0x1FFF];
        // high memory = echo ram, OAM, unusable, IO, HRAM, IE
        case 0xF: 
            return mem_read_high(mem, addr);
    }
    return 0xFF;
}

static void mem_write_high(struct memory *mem, uint16_t addr, uint8_t val) {
    // LY guard
    if (addr == 0xFF44) return;

    // still in echo RAM
    if (addr < 0xFE00) {
        mem->wram[addr & 0x1FFF] = val;
    // OAM
    } else if (addr < 0xFEA0) { 
        mem->oam[addr - 0xFE00] = val;
    // NOT USABLE
    } else if (addr < 0xFF00) { 
        return;
    // IO registers
    } else if (addr < 0xFF80) { 
        mem->io[addr - 0xFF00] = val;
    // HRAM
    } else if (addr < 0xFFFF) {
        mem->hram[addr - 0xFF80] = val;
    // IE register
    } else { 
        mem->ie = val;
    }
    return;
}

void mem_write(struct memory *mem, uint16_t addr, uint8_t val) {
    switch (addr >> 12) {
        // ROM bank 00 & NN
        case 0x0: case 0x1: case 0x2: case 0x3: 
        case 0x4: case 0x5: case 0x6: case 0x7: 
            break;
        // VRAM
        case 0x8: case 0x9:
            mem->vram[addr & 0x1FFF] = val;
            break;
        // TODO: E(xternal)RAM
        case 0xA: case 0xB:
            break;
        // WRAM
        case 0xC: case 0xD:
            mem->wram[addr & 0x1FFF] = val;
            break;
        // Echo RAM
        case 0xE: 
            mem->wram[addr & 0x1FFF] = val;
            break;
        // high memory = echo ram, OAM, unusable, IO, HRAM, IE
        case 0xF: 
            mem_write_high(mem, addr, val);
            break;
    }
    return;
}

int mem_print(struct memory *mem, uint16_t start, size_t length) {
    if ((size_t) start + length > 0x10000) {
        fprintf(stderr, "Memory out of bounds\n");
        return -1;
    }
    for (size_t i = 0; i < (length + 15) / 16; i++) {
        // print addr for line
        fprintf(stdout, "%04X:  ", (uint16_t) (start + (16 * i)));
        // print data in line
        for (size_t j = 0; j < 16 && ((i * 16) + j) < length; j++) {
            uint16_t addr = (uint16_t) (start + (i * 16) + j);
            fprintf(stdout, "%02X ", mem_read(mem, addr));
            if ((j + 1) % 4 == 0) fprintf(stdout, " ");
            if ((j + 1) % 8 == 0) fprintf(stdout, " ");
        }
        fprintf(stdout, "\n");
    }
    return 0;
}
