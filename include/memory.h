#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>
#include <stdint.h>
#include "cart.h"

struct memory {
    struct cart *cart;
    uint8_t vram[0x2000];
    uint8_t wram[0x2000];
    // [ 0xE000, 0xFDFF ] ECHO RAM
    uint8_t oam[0xA0];
    // [ 0xFEA0, 0xFEFF ] NOT USABLE
    uint8_t io[0x80];
    uint8_t hram[0xFFFF - 0xFF80];
    uint8_t ie;
};

void mem_init(struct memory *mem, struct cart *cart);
uint8_t mem_read(struct memory *mem, uint16_t addr);
void mem_write(struct memory *mem, uint16_t addr, uint8_t val);

int mem_print(struct memory *mem, uint16_t start, size_t length);

#endif // MEMORY_H_
