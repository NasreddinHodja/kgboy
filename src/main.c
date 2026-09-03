#include "cart.h"
#include "memory.h"
#include "cpu.h"
#include <stdio.h>

int main(void) {
    struct cart cart;
    struct memory mem;
    struct cpu_registers regs;

    fprintf(stdout, "\n");

    // init
    cart_load(&cart, "roms/tetris.gb");
    cart_print(&cart);
    mem_init(&mem, &cart);
    cpu_init(&regs);
    cpu_skip_boot(&regs, &mem);

    /* while (1) { */
        cpu_step(&regs, &mem);
    /* } */

    /* // set noop at 0x0000 */

    // step
    fprintf(stdout, "\n");

    // print state
    cpu_registers_print(&regs);
    fprintf(stdout, "\n");
    mem_print(&mem, 0, 64);

    // test
    fprintf(stdout, "\n\n");
    mem_write(&mem, 0xFF44, 0x42);
    printf("LY after writing 0x42: %02X\n", mem_read(&mem, 0xFF44));

    return 0;
}

