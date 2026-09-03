#ifndef CART_H_
#define CART_H_

#include <stddef.h>
#include <stdint.h>

struct cart {
    uint8_t *rom;
    size_t size;
    char title[17];
    uint8_t type;
    uint8_t rom_size;
    uint8_t ram_size;
    // TODO: eram
};

int cart_load(struct cart *cart, const char *path);
void cart_free(struct cart *cart);
void cart_print(const struct cart *cart);

#endif // CART_H_
