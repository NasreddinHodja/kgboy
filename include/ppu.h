#ifndef PPU_H_
#define PPU_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bus.h"

#define SCREEN_H 144
#define SCREEN_W 160
#define FIFO_SIZE 8
#define MAX_OBJS 10

enum fetcher_state {
    FETCHER_GET_TILE_NUM = 0,
    FETCHER_GET_TILE_LO,
    FETCHER_GET_TILE_HI,
    FETCHER_SLEEP,
    FETCHER_PUSH,
};

struct obj {
    uint8_t y_byte;
    uint8_t x_byte;
    uint8_t idx;
};

struct fifo_item {
    uint8_t color;
    uint8_t palette;
    uint8_t ob_priority;
    uint8_t bg_priority;
};

struct ppu {
    size_t dots;
    uint8_t fb[SCREEN_H][SCREEN_W];
    size_t frame_count; // NOTE: for debug
    bool frame_ready;

    // fifo/pixel_fetcher/renderer
    struct fifo_item bg_fifo[FIFO_SIZE];
    uint8_t bg_fifo_head;
    uint8_t bg_fifo_len;
    struct fifo_item ob_fifo[FIFO_SIZE];
    uint8_t ob_fifo_head;
    uint8_t ob_fifo_len;
    enum fetcher_state fetcher_state;
    size_t fetcher_dot;
    uint8_t fetcher_x;
    uint8_t discard;
    uint8_t tile_num;
    uint16_t tile_addr;
    uint8_t tile_lo;
    uint8_t tile_hi;
    uint8_t pixel_x;
    struct obj ly_objs[10];
    size_t next_obj;
    uint8_t obj_count;
    bool window_active;
    uint8_t window_line;
    bool wy_triggered;
    bool stat_line;

    // regs
    uint8_t lcdc; // FF40 lcd control
    uint8_t stat; // FF41 lcd status
    uint8_t scy;  // FF42 bg scroll Y
    uint8_t scx;  // FF43 bg scroll X 
    uint8_t ly;   // FF44 curr line
    uint8_t lyc;  // FF45 line compare
    uint8_t bgp;  // FF47 BG palette
    uint8_t obp0; // FF48 OBG pallette 0
    uint8_t obp1; // FF49 OBG pallette 1
    uint8_t wy;   // FF4A window Y
    uint8_t wx;   // FF4B window X
};

void ppu_init(struct ppu *ppu);
void ppu_tick(struct ppu *ppu, size_t cycles, struct bus *bus);
uint8_t ppu_read_r(struct ppu *ppu, uint16_t addr);
void ppu_write_r(struct ppu *ppu, uint16_t addr, uint8_t val, struct bus *bus);

#endif // PPU_H_
