#ifndef MBC1_H_
#define MBC1_H_

#include <stdbool.h>
#include <stdint.h>

struct cart;

struct mbc1 {
    uint8_t bank1;
    uint8_t bank2;
    bool ram_enable;
    bool mode;
    uint16_t rom_bank_mask;
    uint16_t ram_bank_mask;
};

void mbc1_init(struct cart *cart);

#endif // MBC1_H_
