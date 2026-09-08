#include "cart/mbc1.h"
#include "cart/cart.h"

static uint8_t mbc1_read_rom(struct cart *cart, uint16_t addr) {
    const uint8_t mode = cart->mbc.mbc1.mode;

    if (addr < 0x4000) {
        if (mode == 0) {
            return cart->rom[addr & 0x3FFF];
        } else {
            const uint8_t bank =
                (cart->mbc.mbc1.bank2 << 5) & cart->mbc.mbc1.rom_bank_mask;
            const uint32_t rom_addr =
                (addr & 0x3FFF) | (bank << 14);
            return cart->rom[rom_addr];
        }
    } else if (addr < 0x8000) {
        const uint8_t bank =
            ((cart->mbc.mbc1.bank2 << 5) | cart->mbc.mbc1.bank1) &
            cart->mbc.mbc1.rom_bank_mask;
        const uint32_t rom_addr = (bank << 14) | (addr & 0x3FFF);
        return cart->rom[rom_addr];
    }

    return 0xFF;
}

static void mbc1_write_rom(struct cart *cart, uint16_t addr, uint8_t val) {
    if (addr < 0x2000) { // ram enable
        cart->mbc.mbc1.ram_enable = ((val & 0x0F) == 0x0A);
    } else if (addr < 0x4000) { // rom bank number
        const uint8_t new_val = val & 0x1F;
        cart->mbc.mbc1.bank1 = new_val ? new_val : 1;
    } else if (addr < 0x6000) { // ram bank number
        cart->mbc.mbc1.bank2 = val & 0x03;
    } else if (addr < 0x8000) { // banking mode select
        cart->mbc.mbc1.mode = val & 0x01;
    }
}

static uint8_t mbc1_read_ram(struct cart *cart, uint16_t addr) {
    struct mbc1 *m = &cart->mbc.mbc1;

    if (!cart->ram || !m->ram_enable)
        return 0xFF;

    const uint8_t bank = m->mode ? (m->bank2 & m->ram_bank_mask) : 0;
    const uint32_t ram_addr = (bank << 13) | (addr & 0x1FFF);
    return cart->ram[ram_addr];
}

static void mbc1_write_ram(struct cart *cart, uint16_t addr, uint8_t val) {
    struct mbc1 *m = &cart->mbc.mbc1;

    if (!cart->ram || !m->ram_enable)
        return;

    const uint8_t bank = m->mode ? (m->bank2 & m->ram_bank_mask) : 0;
    const uint32_t ram_addr = (bank << 13) | (addr & 0x1FFF);
    cart->ram[ram_addr] = val;
}

void mbc1_init(struct cart *cart) {
    cart->read_rom = mbc1_read_rom;
    cart->write_rom = mbc1_write_rom;
    cart->read_ram = mbc1_read_ram;
    cart->write_ram = mbc1_write_ram;

    const uint32_t rom_banks = cart->rom_bytes / 0x4000;
    const uint32_t ram_banks = cart->ram_bytes / 0x2000;
    cart->mbc.mbc1 = (struct mbc1){
        .bank1 = 1,
        .bank2 = 0,
        .mode = false,
        .ram_enable = false,
        .rom_bank_mask = rom_banks - 1,
        .ram_bank_mask = ram_banks ? ram_banks - 1 : 0,
    };
}
