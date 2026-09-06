#include "timer.h"
#include "bus.h"
#include <stdbool.h>

void timer_init(struct timer *timer) {
    timer->counter = 0xABCC;  // NOTE: guessing
    timer->tima = 0;
    timer->tma = 0; 
    timer->tac = 0xF8;  
}

void timer_tick(struct timer *timer, size_t cycles, struct bus *bus) {
    static const uint8_t speeds[] = {9, 3, 5, 7};
    const uint8_t speed = speeds[timer->tac & 3];

    for (size_t c = 0; c < cycles; c++) {
        uint8_t bitb = (timer->counter >> speed) & 1;
        timer->counter++;
        uint8_t bita = (timer->counter >> speed) & 1;
        bool fell = bitb && !bita;

        // advance time
        if ((timer->tac & (1 << 2)) && fell) {
            timer->tima++;
            if (timer->tima == 0) {
                timer->tima = timer->tma;
                bus_request_interrupt(bus, INT_TIMER);
            }
        }
    }

}

uint8_t timer_read_r(struct timer *timer, uint16_t addr) {
    switch (addr) {
        case 0xFF04:
            return timer->counter >> 8;
        case 0xFF05:
            return timer->tima;
        case 0xFF06:
            return timer->tma;
        case 0xFF07:
            return timer->tac;
        default:
            return 0;
    }
}

void timer_write_r(struct timer *timer, uint16_t addr, uint8_t val) {
    switch (addr) {
        case 0xFF04:
            timer->counter = 0;
            break;
        case 0xFF05:
            timer->tima = val;
            break;
        case 0xFF06:
            timer->tma = val;
            break;
        case 0xFF07:
            timer->tac = val;
            break;
        default:
            break;
    }
}
