#ifndef TIMER_H_
#define TIMER_H_

#include "bus.h"
#include <stddef.h>
#include <stdint.h>

struct timer {
    uint16_t counter;  
    uint8_t tima; // FF05: the counter
    uint8_t tma; // FF06: reload val
    uint8_t tac;  // FF07: bits 0-1 picks speed (00:1024 01:16 10:64 11:25)
    // FF04: div = counter >> 8
};

void timer_init(struct timer *timer);
void timer_tick(struct timer *timer, size_t cycles, struct bus *bus);
uint8_t timer_read_r(struct timer *timer, uint16_t addr);
void timer_write_r(struct timer *timer, uint16_t addr, uint8_t val);

#endif // TIMER_H_
