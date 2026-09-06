#ifndef PPU_H_
#define PPU_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bus.h"

#define SCREEN_H 144
#define SCREEN_W 160

struct ppu {
    size_t dots;
    uint8_t fb[SCREEN_H][SCREEN_W];
    size_t frame_count; // NOTE: for debug
    bool frame_ready;

    // regs
    uint8_t lcdc; // FF40 lcd control
    uint8_t stat; // FF41 lcd status
    uint8_t scy;  // FF42 bg scroll Y
    uint8_t scx;  // FF43 bg scroll X 
    uint8_t ly;   // FF44 curr line
    uint8_t lyc;  // FF45 line compare
    uint8_t dma;  // FF46 OAM DMA start
    uint8_t bgp;  // FF47 BG palette
    uint8_t obp0; // FF48 OBG pallette 0
    uint8_t obp1; // FF49 OBG pallette 1
    uint8_t wy;   // FF4A window Y
    uint8_t wx;   // FF4B window X
};

void ppu_init(struct ppu *ppu);
void ppu_tick(struct ppu *ppu, size_t cycles, struct bus *bus);
uint8_t ppu_read_r(struct ppu *ppu, uint16_t addr);
void ppu_write_r(struct ppu *ppu, uint16_t addr, uint8_t val);

#endif // PPU_H_
