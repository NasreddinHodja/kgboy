#include "display.h"
#include "SDL_events.h"
#include "ppu.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

bool display_init(struct display *display) {
    memset(display, 0, sizeof(*display));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "error initializing SDL: %s\n", SDL_GetError());
        display_destroy(display);
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    display->win =
        SDL_CreateWindow("KGBoy", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, SCREEN_W * 4, SCREEN_H * 4, 0);
    if (!display->win) {
        fprintf(stderr, "error initializing SDL window: %s\n", SDL_GetError());
        display_destroy(display);
        return false;
    }

    display->ren = SDL_CreateRenderer(
        display->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!display->ren) {
        fprintf(stderr, "error initializing SDL rederer: %s\n", SDL_GetError());
        display_destroy(display);
        return false;
    }

    display->tex =
        SDL_CreateTexture(display->ren, SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    if (!display->tex) {
        fprintf(stderr, "error initializing SDL texture: %s\n", SDL_GetError());
        display_destroy(display);
        return false;
    }

    return true;
}

void display_destroy(struct display *display) {
    if (display->tex) {
        SDL_DestroyTexture(display->tex);
        display->tex = NULL;
    }
    if (display->ren) {
        SDL_DestroyRenderer(display->ren);
        display->tex = NULL;
    }
    if (display->win) {
        SDL_DestroyWindow(display->win);
        display->tex = NULL;
    }
    SDL_Quit();
}

void display_present(struct display *display, const uint8_t (*fb)[SCREEN_W],
                     uint8_t bgp) {
    for (size_t line = 0; line < SCREEN_H; line++) {
        for (size_t col = 0; col < SCREEN_W; col++) {
            const uint8_t shade = (bgp >> (fb[line][col] * 2)) & 3;
            unsigned char r = 224;
            unsigned char g = 248;
            unsigned char b = 208;
            switch (shade) {
            case 0: // white
                break;
            case 1: // light gray
                r = 136;
                g = 192;
                b = 112;
                break;
            case 2: // dark gray
                r = 52;
                g = 104;
                b = 86;
                break;
            case 3: // black
                r = 8;
                g = 24;
                b = 32;
                break;
            }
            display->pixels[line * SCREEN_W + col] =
                0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    SDL_UpdateTexture(display->tex, NULL, display->pixels,
                      SCREEN_W * sizeof(uint32_t));
    SDL_RenderClear(display->ren);
    SDL_RenderCopy(display->ren, display->tex, NULL, NULL);
    SDL_RenderPresent(display->ren);
}

bool display_poll() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
    }
    return true;
}
