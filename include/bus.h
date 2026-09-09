#ifndef BUS_H_
#define BUS_H_

#include "cart/cart.h"
#include <stddef.h>
#include <stdint.h>

enum interrupt {
    INT_VBLANK,
    INT_STAT,
    INT_TIMER,
    INT_SERIAL,
    INT_JOYPAD,
};

struct bus {
    struct cart *cart;
    struct ppu *ppu;
    struct timer *timer;
    struct joypad *jp;

    uint8_t vram[0x2000];
    uint8_t wram[0x2000];
    // [ 0xE000, 0xFDFF ] ECHO RAM
    uint8_t oam[0xA0];
    // [ 0xFEA0, 0xFEFF ] NOT USABLE
    uint8_t io[0x80];
    uint8_t hram[0xFFFF - 0xFF80];
    uint8_t ie;
};

void bus_mem_init(struct bus *bus, struct cart *cart, struct ppu *ppu,
                  struct timer *timer, struct joypad *jp);
uint8_t bus_mem_read8(struct bus *bus, uint16_t addr);
void bus_mem_write8(struct bus *bus, uint16_t addr, uint8_t val);
uint16_t bus_mem_read16(struct bus *bus, uint16_t addr);
void bus_mem_write16(struct bus *bus, uint16_t addr, uint16_t val);

int bus_mem_print(struct bus *bus, uint16_t start, size_t length);

void bus_request_interrupt(struct bus *bus, enum interrupt intr);
#endif // BUS_H_
