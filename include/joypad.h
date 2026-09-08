#ifndef JOYPAD_H_
#define JOYPAD_H_

#include <stdint.h>

struct joypad {
    uint8_t buttons; // 0:A, 1:B, 2:select, 3:start
    uint8_t dpad;    // 0:r, 1:l, 2:u, 3:d
    uint8_t select;
};

void joypad_init(struct joypad *jp);
uint8_t joypad_read(struct joypad *jp);
void joypad_write(struct joypad *jp, uint8_t val);


#endif // JOYPAD_H_
