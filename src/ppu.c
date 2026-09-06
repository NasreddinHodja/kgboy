#include "ppu.h"
#include "bus.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ppu_init(struct ppu *ppu) {
    ppu->dots = 0;
    memset(ppu->fb, 0, sizeof(ppu->fb));
    ppu->frame_count = 0; // NOTE: debug info
    ppu->frame_ready = false;

    ppu->lcdc = 0x91;
    ppu->stat = 0x85;
    ppu->scy = 0x00;
    ppu->scx = 0x00;
    ppu->ly = 0x00;
    ppu->lyc = 0x00;
    ppu->dma = 0xFF;
    ppu->bgp = 0xFC;
    ppu->obp0 = 0x00;
    ppu->obp1 = 0x00;
    ppu->wy = 0x00;
    ppu->wx = 0x00;
}

static void ppu_set_mode(struct ppu *ppu, uint8_t m) {
    ppu->stat = (ppu->stat & 0xFC) | m;
}

/* static uint8_t ppu_get_mode(struct ppu *ppu) { */
/*     return ppu->stat & 0x03; */
/* } */

/* static void ppu_fb_dump_ppm(struct ppu *ppu) { */
/*     char file_name[32]; */
/*     snprintf(file_name, sizeof(file_name), "frame-%zu.ppm", ppu->frame_count); */

/*     FILE *fp = fopen(file_name, "wb"); */
/*     if (fp == NULL) { */
/*         perror("fopen"); */
/*         exit(1); */
/*     } */

/*     fprintf(fp, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H); */

/*     for (size_t line = 0; line < SCREEN_H; line++) { */
/*         for (size_t col = 0; col < SCREEN_W; col++) { */
/*             const uint8_t color = (ppu->bgp >> (ppu->fb[line][col] * 2)) & 3; */
/*             unsigned char r = 224; */
/*             unsigned char g = 248; */
/*             unsigned char b = 208; */
/*             switch (color) { */
/*             case 0: // white */
/*                 break; */
/*             case 1: // light gray */
/*                 r = 136; */
/*                 g = 192; */
/*                 b = 112; */
/*                 break; */
/*             case 2: // dark gray */
/*                 r = 52; */
/*                 g = 104; */
/*                 b = 86; */
/*                 break; */
/*             case 3: // black */
/*                 r = 8; */
/*                 g = 24; */
/*                 b = 32; */
/*                 break; */
/*             } */
/*             fputc(r, fp); */
/*             fputc(g, fp); */
/*             fputc(b, fp); */
/*         } */
/*     } */

/*     fclose(fp); */
/* } */

static void ppu_frame_done(struct ppu *ppu) {
    ppu->frame_ready = true;
    ppu->frame_count++;
}

static void ppu_render_line(struct ppu *ppu, struct bus *bus) {
    const uint8_t scrolled_l = (ppu->scy + ppu->ly) & 255;
    const uint8_t tilemap_l = scrolled_l / 8;

    for (size_t col = 0; col < SCREEN_W; col++) {
        const uint8_t scrolled_c = (ppu->scx + col) & 255;
        const uint8_t tilemap_c = scrolled_c / 8;

        const uint8_t tile_idx =
            bus_mem_read8(bus, 0x9800 + tilemap_l * 32 + tilemap_c);
        const uint16_t tile_addr = 0x8000 + tile_idx * 16;
        const uint16_t tile_row_addr = tile_addr + (scrolled_l % 8) * 2;

        const uint8_t lo = bus_mem_read8(bus, tile_row_addr);
        const uint8_t hi = bus_mem_read8(bus, tile_row_addr + 1);

        // extract 1 pix
        const uint8_t bit = 7 - (scrolled_c % 8);
        const uint8_t color = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);

        ppu->fb[ppu->ly][col] = color;
    }
}

void ppu_tick(struct ppu *ppu, size_t cycles, struct bus *bus) {
    // 1. dots += cycles
    // 2. check mode boundaries
    // 3. if in mode 3, draw line (ppu_draw_line())
    // fixed modes len:
    // dots:  0            80                252              456
    //        |   mode 2   |     mode 3      |     mode 0     |
    //        ^            ^                 ^                ^
    //        line starts  render here       hblank           next line
    //        ....
    // ly: 144 - mode 1

    if (!(ppu->lcdc & (1 << 7))) {
        ppu->ly = 0;
        ppu_set_mode(ppu, 0);
        ppu->dots = 0;
        return;
    }

    for (size_t c = 0; c < cycles; c++) {
        ppu->dots++;

        if (ppu->ly < 144) {
            if (ppu->dots == 80) {
                ppu_set_mode(ppu, 3);
                ppu_render_line(ppu, bus);
            } else if (ppu->dots == 252) {
                ppu_set_mode(ppu, 0);
            }
        }

        if (ppu->dots == 456) {
            ppu->dots = 0;
            ppu->ly++;

            if (ppu->ly == 144) {
                ppu_set_mode(ppu, 1);
                bus_request_interrupt(bus, INT_VBLANK);
                ppu_frame_done(ppu);
            } else if (ppu->ly == 154) {
                ppu->ly = 0;
                ppu_set_mode(ppu, 2);
            } else if (ppu->ly < 144) {
                ppu_set_mode(ppu, 2);
            }
        }
    }
}

uint8_t ppu_read_r(struct ppu *ppu, uint16_t addr) {
    switch (addr) {
    case 0xFF40:
        return ppu->lcdc;
    case 0xFF41:
        return ppu->stat;
    case 0xFF42:
        return ppu->scy;
    case 0xFF43:
        return ppu->scx;
    case 0xFF44:
        return ppu->ly;
    case 0xFF45:
        return ppu->lyc;
    case 0xFF46:
        return ppu->dma;
    case 0xFF47:
        return ppu->bgp;
    case 0xFF48:
        return ppu->obp0;
    case 0xFF49:
        return ppu->obp1;
    case 0xFF4A:
        return ppu->wy;
    case 0xFF4B:
        return ppu->wx;
    default:
        return 0xFF;
    }
}

void ppu_write_r(struct ppu *ppu, uint16_t addr, uint8_t val) {
    // LY guard
    if (addr == 0xFF44)
        return;

    switch (addr) {
    case 0xFF40:
        ppu->lcdc = val;
        break;
    case 0xFF41:
        ppu->stat = (val & 0x78) | (ppu->stat & 0x87); // only 3-6 are writable
        break;
    case 0xFF42:
        ppu->scy = val;
        break;
    case 0xFF43:
        ppu->scx = val;
        break;
    case 0xFF44:
        ppu->ly = val;
        break;
    case 0xFF45:
        ppu->lyc = val;
        break;
    case 0xFF46:
        // TODO: should trigger OAM DMA
        // => copy 160 bytes to OAM
        ppu->dma = val;
        break;
    case 0xFF47:
        ppu->bgp = val;
        break;
    case 0xFF48:
        ppu->obp0 = val;
        break;
    case 0xFF49:
        ppu->obp1 = val;
        break;
    case 0xFF4A:
        ppu->wy = val;
        break;
    case 0xFF4B:
        ppu->wx = val;
        break;
    default:
        break;
    }
    return;
}
