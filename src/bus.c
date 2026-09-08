#include "bus.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bus_mem_init(struct bus *bus, struct cart *cart, struct ppu *ppu,
                  struct timer *timer, struct joypad *jp) {
    memset(bus, 0, sizeof(*bus));
    bus->cart = cart;
    bus->ppu = ppu;
    bus->timer = timer;
    bus->jp = jp;
}

static uint8_t bus_mem_read_n_high(struct bus *bus, uint16_t addr) {
    // still in echo RAM
    if (addr < 0xFE00)
        return bus->wram[addr & 0x1FFF];
    // OAM
    if (addr < 0xFEA0) {
        return bus->oam[addr - 0xFE00];
        // joypad
    } else if (addr == 0xFF00) {
        return joypad_read(bus->jp);
        // NOT USABLE
    } else if (addr < 0xFF00) {
        return 0xFF;
        // IO registers
    } else if (addr == 0xFF0F) {
        return bus->io[0x0F] | 0xE0; // IE top 3 bits are always 1
    } else if (addr >= 0xFF04 && addr <= 0xFF07) {
        return timer_read_r(bus->timer, addr);
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        return ppu_read_r(bus->ppu, addr);
    } else if (addr < 0xFF80) {
        return bus->io[addr - 0xFF00];
        // HRAM
    } else if (addr < 0xFFFF) {
        return bus->hram[addr - 0xFF80];
        // IE register
    } else {
        return bus->ie;
    }
    return 0xFF;
}

uint8_t bus_mem_read8(struct bus *bus, uint16_t addr) {
    switch (addr >> 12) {
    // ROM bank 00
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    // ROM bank NN
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        return bus->cart->read_rom(bus->cart, addr);
    // VRAM
    case 0x8:
    case 0x9:
        return bus->vram[addr & 0x1FFF];
    case 0xA:
        return bus->cart->read_ram(bus->cart, addr);
    case 0xB:
        return 0xFF;
    // WRAM
    case 0xC:
    case 0xD:
    // Echo RAM
    case 0xE:
        return bus->wram[addr & 0x1FFF];
    // high memory = echo ram, OAM, unusable, IO, HRAM, IE
    case 0xF:
        return bus_mem_read_n_high(bus, addr);
    }
    return 0xFF;
}

uint16_t bus_mem_read16(struct bus *bus, uint16_t addr) {
    return bus_mem_read8(bus, addr) | (bus_mem_read8(bus, addr + 1) << 8);
}

static void bus_oam_dma_transfer(struct bus *bus, uint8_t val) {
    const uint16_t src = val << 8;
    for (size_t i = 0; i < 0xA0; i++)
        bus->oam[i] = bus_mem_read8(bus, src + i);
}

static void bus_mem_write_n_high(struct bus *bus, uint16_t addr, uint8_t val) {
    // still in echo RAM
    if (addr < 0xFE00) {
        bus->wram[addr & 0x1FFF] = val;
        // OAM
    } else if (addr < 0xFEA0) {
        bus->oam[addr - 0xFE00] = val;
        // NOT USABLE
    } else if (addr < 0xFF00) {
        return;
    } else if (addr == 0xFF02 && (val & (1 << 7))) { // serial
        putchar(bus->io[0x01]);
        fflush(stdout);
        bus->io[0x02] = val & ~(1 << 7);
        bus_request_interrupt(bus, INT_SERIAL);
    } else if (addr >= 0xFF04 && addr <= 0xFF07) { // timer
        timer_write_r(bus->timer, addr, val);
        // IO registers
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        if (addr == 0xFF46)
            bus_oam_dma_transfer(bus, val);
        else
            ppu_write_r(bus->ppu, addr, val);
        // OAM DAM transfer
    } else if (addr < 0xFF80) {
        bus->io[addr - 0xFF00] = val;
        // HRAM
    } else if (addr < 0xFFFF) {
        bus->hram[addr - 0xFF80] = val;
        // IE register
    } else {
        bus->ie = val;
    }
    return;
}

void bus_mem_write8(struct bus *bus, uint16_t addr, uint8_t val) {
    switch (addr >> 12) {
    // ROM bank 00 & NN
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    // ROM bank NN
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        bus->cart->write_rom(bus->cart, addr, val);
        break;
    // VRAM
    case 0x8:
    case 0x9:
        bus->vram[addr & 0x1FFF] = val;
        break;
    case 0xA:
        bus->cart->write_ram(bus->cart, addr, val);
        break;
    case 0xB:
        break;
    // WRAM
    case 0xC:
    case 0xD:
        bus->wram[addr & 0x1FFF] = val;
        break;
    // Echo RAM
    case 0xE:
        bus->wram[addr & 0x1FFF] = val;
        break;
    // high memory = echo ram, OAM, unusable, IO, HRAM, IE
    case 0xF:
        bus_mem_write_n_high(bus, addr, val);
        break;
    }
    return;
}

void bus_mem_write16(struct bus *bus, uint16_t addr, uint16_t val) {
    bus_mem_write8(bus, addr, val & 0xFF);
    bus_mem_write8(bus, addr + 1, val >> 8);
}

int bus_mem_print(struct bus *bus, uint16_t start, size_t length) {
    if ((size_t)start + length > 0x10000) {
        fprintf(stderr, "Memory out of bounds\n");
        return -1;
    }

    for (size_t i = 0; i < (length + 15) / 16; i++) {
        // print addr for line
        fprintf(stdout, "%04X:  ", (uint16_t)(start + (16 * i)));
        // print data in line
        for (size_t j = 0; j < 16 && ((i * 16) + j) < length; j++) {
            uint16_t addr = (uint16_t)(start + (i * 16) + j);
            fprintf(stdout, "%02X ", bus_mem_read8(bus, addr));
            if ((j + 1) % 4 == 0)
                fprintf(stdout, " ");
            if ((j + 1) % 8 == 0)
                fprintf(stdout, " ");
        }
        fprintf(stdout, "\n");
    }
    return 0;
}

void bus_request_interrupt(struct bus *bus, enum Interrupt intr) {
    bus->io[0x0F] |= (1 << intr);
}
