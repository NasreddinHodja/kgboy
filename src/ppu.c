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

    ppu->obj_count = 0;
    ppu->next_obj = 0;
    ppu->bg_fifo_head = 0;
    ppu->bg_fifo_head = 0;
    memset(ppu->bg_fifo, 0, sizeof(ppu->bg_fifo));
    ppu->ob_fifo_head = 0;
    ppu->ob_fifo_len = 0;
    memset(ppu->bg_fifo, 0, sizeof(ppu->ob_fifo));
    memset(ppu->ly_objs, 0, sizeof(ppu->ly_objs));
    ppu->fetcher_dot = 0;
    ppu->fetcher_state = FETCHER_GET_TILE_NUM;
    ppu->fetcher_dot = 0;
    ppu->fetcher_x = 0;
    ppu->discard = 0;
    ppu->tile_addr = 0;
    ppu->tile_num = 0;
    ppu->tile_lo = 0;
    ppu->tile_hi = 0;
    ppu->pixel_x = 0;
    ppu->window_line = 0;
    ppu->wy_triggered = false;

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

static uint8_t ppu_get_mode(struct ppu *ppu) {
    return ppu->stat & 0x03;
}

static void ppu_set_mode(struct ppu *ppu, uint8_t m) {
    ppu->stat = (ppu->stat & 0xFC) | m;
}

static void ppu_recompute_stat_line(struct ppu *ppu, struct bus *bus) {
    const bool prev_line = ppu->stat_line;
    ppu->stat_line = (((ppu->stat >> 3) & 1) && ppu_get_mode(ppu) == 0)
        || (((ppu->stat >> 4) & 1) && ppu_get_mode(ppu) == 1)
        || (((ppu->stat >> 5) & 1) && ppu_get_mode(ppu) == 2)
        || (((ppu->stat >> 6) & 1) && ppu->ly == ppu->lyc);
    if (ppu->stat_line && !prev_line) bus_request_interrupt(bus, INT_STAT);
}

/* static void ppu_fb_dump_ppm(struct ppu *ppu) { */
/*     char file_name[32]; */
/*     snprintf(file_name, sizeof(file_name), "frame-%zu.ppm",
 * ppu->frame_count); */

/*     FILE *fp = fopen(file_name, "wb"); */
/*     if (fp == NULL) { */
/*         perror("fopen"); */
/*         exit(1); */
/*     } */

/*     fprintf(fp, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H); */

/*     for (size_t line = 0; line < SCREEN_H; line++) { */
/*         for (size_t col = 0; col < SCREEN_W; col++) { */
/*             const uint8_t color = (ppu->bgp >> (ppu->fb[line][col] * 2)) & 3;
 */
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

static void ppu_fetcher_tick(struct ppu *ppu, struct bus *bus) {
    if (ppu->fetcher_dot == 0) {
        ppu->fetcher_dot = 1;
        if (ppu->fetcher_state != FETCHER_PUSH)
            return;
    } else {
        ppu->fetcher_dot = 0;
    }

    switch (ppu->fetcher_state) {
    case FETCHER_GET_TILE_NUM: {
        uint16_t tilemap;
        uint8_t tile_col;
        uint8_t tile_row;
        if (ppu->window_active) {
            tilemap = ((ppu->lcdc >> 6) & 1) ? 0x9C00 : 0x9800;
            tile_col = ppu->fetcher_x;
            tile_row = ppu->window_line / 8;
        } else {
            tilemap = ((ppu->lcdc >> 3) & 1) ? 0x9C00 : 0x9800;
            tile_col = ((ppu->scx / 8) + ppu->fetcher_x) & 0x1F;
            tile_row = ((ppu->ly + ppu->scy) & 0xFF) / 8;
        }
        const uint16_t tile_addr = tilemap + tile_row * 32 + tile_col;
        ppu->tile_num = bus_mem_read8(bus, tile_addr);
        ppu->fetcher_state = FETCHER_GET_TILE_LO;
        break;
    }
    case FETCHER_GET_TILE_LO: {
        const uint16_t tile_addr = (ppu->lcdc >> 4) & 1
                                       ? 0x8000 + ppu->tile_num * 16
                                       : 0x9000 + (int8_t)ppu->tile_num * 16;
        const uint8_t tile_row = ppu->window_active ? ppu->window_line & 7
                                                    : (ppu->ly + ppu->scy) & 7;
        const uint16_t addr = tile_addr + tile_row * 2;
        ppu->tile_addr = addr;
        ppu->tile_lo = bus_mem_read8(bus, addr);
        ppu->fetcher_state = FETCHER_GET_TILE_HI;
        break;
    }
    case FETCHER_GET_TILE_HI: {
        ppu->tile_hi = bus_mem_read8(bus, ppu->tile_addr + 1);
        ppu->fetcher_state = FETCHER_SLEEP;
        break;
    }
    case FETCHER_SLEEP:
        ppu->fetcher_state = FETCHER_PUSH;
        break;
    case FETCHER_PUSH:
        if (ppu->bg_fifo_len != 0)
            return;
        while (ppu->bg_fifo_len < FIFO_SIZE) {
            const uint8_t bit = FIFO_SIZE - 1 - ppu->bg_fifo_len;
            const uint8_t color =
                ((ppu->tile_hi >> bit) & 1) << 1 | ((ppu->tile_lo >> bit) & 1);
            ppu->bg_fifo[ppu->bg_fifo_len++] = (struct fifo_item){
                .color = color,
                .palette = 0,
                .ob_priority = 0,
            };
        }
        ppu->bg_fifo_head = 0;
        ppu->fetcher_x++;
        ppu->fetcher_state = FETCHER_GET_TILE_NUM;
        break;
    }
}

/* static void ppu_render_line(struct ppu *ppu, struct bus *bus) { */
/*     const uint8_t scrolled_l = (ppu->scy + ppu->ly) & 255; */
/*     const uint8_t tilemap_l = scrolled_l / 8; */

/*     for (size_t col = 0; col < SCREEN_W; col++) { */
/*         const uint8_t scrolled_c = (ppu->scx + col) & 255; */
/*         const uint8_t tilemap_c = scrolled_c / 8; */

/*         const uint8_t tile_idx = */
/*             bus_mem_read8(bus, 0x9800 + tilemap_l * 32 + tilemap_c); */
/*         const uint16_t tile_addr = 0x8000 + tile_idx * 16; */
/*         const uint16_t tile_row_addr = tile_addr + (scrolled_l % 8) * 2; */

/*         const uint8_t lo = bus_mem_read8(bus, tile_row_addr); */
/*         const uint8_t hi = bus_mem_read8(bus, tile_row_addr + 1); */

/*         // extract 1 pix */
/*         const uint8_t bit = 7 - (scrolled_c % 8); */
/*         const uint8_t color = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
 */
/*         const uint8_t shade = (ppu->bgp >> (color * 2)) & 3; */

/*         ppu->fb[ppu->ly][col] = shade; */
/*     } */
/* } */

void oam_scan(struct ppu *ppu, struct bus *bus) {
    const uint8_t size = ((ppu->lcdc >> 2) & 0x01) ? 16 : 8;
    ppu->obj_count = 0;

    for (size_t i = 0; i < (sizeof(bus->oam) / 4); i++) {
        if (ppu->obj_count >= 10)
            break;
        const uint8_t y_byte = bus->oam[i * 4];
        const uint8_t x_byte = bus->oam[i * 4 + 1];

        const uint8_t row = ppu->ly - (y_byte - 16);
        if (row < size) { // don't need to check row > 0
            // insert in x order
            size_t j = 0;
            while (j < ppu->obj_count && ppu->ly_objs[j].x_byte <= x_byte)
                j++;
            memmove(&ppu->ly_objs[j + 1], &ppu->ly_objs[j],
                    (ppu->obj_count - j) * sizeof(struct obj));

            ppu->ly_objs[j] = (struct obj){
                .y_byte = y_byte,
                .x_byte = x_byte,
                .idx = i,
            };
            ppu->obj_count++;
        }
    }
}

static void ppu_pop_and_render(struct ppu *ppu, struct bus *bus) {
    if (ppu->wx >= 7 && ppu->pixel_x == ppu->wx - 7 && (ppu->lcdc & (1 << 5)) &&
        ppu->wy_triggered && !ppu->window_active) {
        ppu->window_active = true;
        ppu->bg_fifo_len = 0;
        ppu->bg_fifo_head = 0;
        ppu->fetcher_state = FETCHER_GET_TILE_NUM;
        ppu->fetcher_dot = 0;
        ppu->fetcher_x = 0;
    }

    if (ppu->bg_fifo_len == 0)
        return;

    // fill ob_fifo
    if (ppu->lcdc & 2) {
        while (ppu->next_obj < ppu->obj_count) {
            const int screen_x = ppu->ly_objs[ppu->next_obj].x_byte - 8;
            const uint8_t start = screen_x < 0 ? 0 : screen_x;
            if (ppu->pixel_x != start) break;

            const struct obj obj = ppu->ly_objs[ppu->next_obj];
            size_t tile_idx = bus->oam[obj.idx * 4 + 2];
            if (ppu->lcdc & (1 << 2))
                tile_idx &= 0xFE;
            const size_t attrs = bus->oam[obj.idx * 4 + 3];
            const uint8_t height = (ppu->lcdc & (1 << 2)) ? 16 : 8;
            uint8_t row = ppu->ly - (obj.y_byte - 16);
            if (attrs & (1 << 6))
                row = height - 1 - row;
            const uint16_t addr = 0x8000 + tile_idx * 16 + row * 2;

            const uint8_t lo = bus_mem_read8(bus, addr);
            const uint8_t hi = bus_mem_read8(bus, addr + 1);

            // pad ob_fifo
            while (ppu->ob_fifo_len < 8) {
                ppu->ob_fifo[(ppu->ob_fifo_head + ppu->ob_fifo_len) &
                             (FIFO_SIZE - 1)] = (struct fifo_item){
                    .color = 0,
                    .palette = 0,
                    .ob_priority = 0,

                };
                ppu->ob_fifo_len++;
            }

            // fill ob_fifo
            for (size_t i = 0; i < 8; i++) {
                const uint8_t bit = attrs & 0x20 ? i : 7 - i;
                const uint8_t color =
                    ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
                const struct fifo_item *slot =
                    &ppu->ob_fifo[(ppu->ob_fifo_head + i) & (FIFO_SIZE - 1)];
                if (color != 0 && slot->color == 0)
                    ppu->ob_fifo[(ppu->ob_fifo_head + i) & (FIFO_SIZE - 1)] =
                        (struct fifo_item){.color = color,
                                           .palette = !!(attrs & (1 << 4)),
                                           .bg_priority = !!(attrs & (1 << 7))};
            }
            ppu->next_obj++;
        }
    }

    struct fifo_item ob_item;
    struct fifo_item bg_item = ppu->bg_fifo[ppu->bg_fifo_head];
    if (!(ppu->lcdc & 1)) bg_item.color = (ppu->bgp & 3);

    ppu->bg_fifo_head = (ppu->bg_fifo_head + 1) & (FIFO_SIZE - 1);
    ppu->bg_fifo_len--;
    bool ob_win = false;

    if (ppu->ob_fifo_len > 0) {
        ob_item = ppu->ob_fifo[ppu->ob_fifo_head];
        ppu->ob_fifo_head = (ppu->ob_fifo_head + 1) & (FIFO_SIZE - 1);
        ppu->ob_fifo_len--;
        if (ob_item.color != 0 && (ppu->lcdc & (1 << 1)) &&
            (!ob_item.bg_priority || bg_item.color == 0))
            ob_win = true;
    }

    if (ppu->discard > 0) {
        ppu->discard--;
        return;
    }

    const uint8_t color = ob_win ? ob_item.color : bg_item.color;
    const uint8_t palette =
        ob_win ? (ob_item.palette ? ppu->obp1 : ppu->obp0) : ppu->bgp;

    ppu->fb[ppu->ly][ppu->pixel_x] = (palette >> (color * 2)) & 3;

    ppu->pixel_x++;
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
        ppu->stat |= (ppu->ly == ppu->lyc) << 2;
        ppu_set_mode(ppu, 0);
        ppu_recompute_stat_line(ppu, bus);
        ppu->dots = 0;
        return;
    }

    for (size_t c = 0; c < cycles; c++) {
        ppu->dots++;

        ppu->stat |= (ppu->ly == ppu->lyc) << 2;

        if (ppu->ly < 144) {
            if (ppu->dots == 1) { // mode 2
                oam_scan(ppu, bus);
                if (ppu->ly == ppu->wy && !ppu->wy_triggered)
                    ppu->wy_triggered = true;
            } else if (ppu->dots == 80) { // reset for mode 3
                ppu->window_active = false;
                ppu_set_mode(ppu, 3);
                ppu_recompute_stat_line(ppu, bus);
                ppu->bg_fifo_len = 0;
                ppu->bg_fifo_head = 0;
                ppu->fetcher_state = FETCHER_GET_TILE_NUM;
                ppu->fetcher_dot = 0;
                ppu->fetcher_x = 0;
                ppu->pixel_x = 0;
                ppu->discard = ppu->scx & 7;
                ppu->ob_fifo_head = 0;
                ppu->ob_fifo_len = 0;
                ppu->next_obj = 0;
            } else if (ppu->dots > 80 && ppu->pixel_x < 160) { // mode 3
                ppu_pop_and_render(ppu, bus);
                ppu_fetcher_tick(ppu, bus);
            } else if (ppu->pixel_x == 160) { // mode 0
                ppu_set_mode(ppu, 0);
                ppu_recompute_stat_line(ppu, bus);
            }
        }

        if (ppu->dots == 456) {
            if (ppu->window_active) ppu->window_line++;
            ppu->dots = 0;
            ppu->ly++;
            ppu_recompute_stat_line(ppu, bus);
            ppu->stat |= (ppu->ly == ppu->lyc) << 2;


            if (ppu->ly == 144) {
                ppu_set_mode(ppu, 1);
                ppu_recompute_stat_line(ppu, bus);
                bus_request_interrupt(bus, INT_VBLANK);
                ppu->wy_triggered = false;
                ppu_frame_done(ppu);
            } else if (ppu->ly == 154) {
                ppu->ly = 0;
                ppu->stat |= (ppu->ly == ppu->lyc) << 2;
                ppu->window_line = 0;
                ppu_set_mode(ppu, 2);
                ppu_recompute_stat_line(ppu, bus);
            } else if (ppu->ly < 144) {
                ppu_set_mode(ppu, 2);
                ppu_recompute_stat_line(ppu, bus);
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

void ppu_write_r(struct ppu *ppu, uint16_t addr, uint8_t val, struct bus *bus) {
    // LY guard
    if (addr == 0xFF44)
        return;

    switch (addr) {
    case 0xFF40:
        ppu->lcdc = val;
        break;
    case 0xFF41:
        ppu->stat = (val & 0x78) | (ppu->stat & 0x87); // only 3-6 are writable
        ppu_recompute_stat_line(ppu, bus);
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
        ppu->stat |= (ppu->ly == ppu->lyc) << 2;
        ppu_recompute_stat_line(ppu, bus);
        break;
    case 0xFF46: // OAM DMA triggered in bus
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
