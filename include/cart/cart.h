#ifndef CART_H_
#define CART_H_

#include "cart/mbc1.h"
#include <stddef.h>
#include <stdint.h>

struct cart {
    char title[17];
    uint8_t type;
    uint8_t rom_code;
    uint8_t ram_code;

    uint8_t *rom;
    size_t rom_bytes;
    uint8_t *ram;
    size_t ram_bytes;

    // mbc
    union {
        struct mbc1 mbc1;
    } mbc;
    uint8_t (*read_rom)(struct cart *, uint16_t);
    void (*write_rom)(struct cart *, uint16_t, uint8_t);
    uint8_t (*read_ram)(struct cart *, uint16_t);
    void (*write_ram)(struct cart *, uint16_t, uint8_t);
};

int cart_load(struct cart *cart, const char *path);
void cart_free(struct cart *cart);
void cart_print(const struct cart *cart);

#endif // CART_H_
