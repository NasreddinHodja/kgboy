#include "cart/mbc0.h"

static uint8_t mbc0_read_rom(struct cart *cart, uint16_t addr) {
    return cart->rom[addr];
}

static void mbc0_write_rom(struct cart *cart, uint16_t addr, uint8_t val) {
    (void)cart; (void)addr; (void)val;
}

static uint8_t mbc0_read_ram(struct cart *cart, uint16_t addr) {
    if (!cart->ram) return 0xFF;
    return cart->ram[addr - 0xA000];
}

static void mbc0_write_ram(struct cart *cart, uint16_t addr, uint8_t val) {
    if (!cart->ram) return;
    cart->ram[addr - 0xA000] = val;
}
    

void mbc0_init(struct cart *cart) {
    cart->read_rom = mbc0_read_rom;
    cart->write_rom = mbc0_write_rom;
    cart->read_ram = mbc0_read_ram;
    cart->write_ram = mbc0_write_ram;
}
