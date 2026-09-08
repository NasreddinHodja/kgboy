#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "joypad.h"
#include "ppu.h"
#include <SDL.h>
#include <stdbool.h>

struct display {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;

    uint32_t pixels[SCREEN_H * SCREEN_W];
};

bool display_init(struct display *display);
void display_present(struct display *display, const uint8_t fb[144][160],
                     uint8_t bgp);
bool display_poll(struct joypad *jp);
void display_destroy(struct display *display);

#endif // DISPLAY_H_
