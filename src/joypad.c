#include "joypad.h"

uint8_t joypad_read(struct joypad *jp) {
    uint8_t nib = 0x0F;
    if (!(jp->select & 0x10))
        nib &= jp->dpad;
    if (!(jp->select & 0x20))
        nib &= jp->buttons;
    return 0xC0 | jp->select | nib;
}

void joypad_write(struct joypad *jp, uint8_t val) { jp->select = val & 0x30; }

void joypad_init(struct joypad *jp) {
    jp->buttons = 0x0F;
    jp->dpad = 0x0F;
    jp->select = 0x00;
}
