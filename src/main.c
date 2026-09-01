#include "memory.h"
#include "cpu.h"
#include <stdio.h>

int main(void) {
    struct memory mem;
    struct cpu_registers regs;

    fprintf(stdout, "\n");

    // init
    mem_init(&mem);
    cpu_init(&regs);
    cpu_skip_boot(&regs, &mem);

    // set noop at 0x0000

    // step
    cpu_step(&regs, &mem);
    fprintf(stdout, "\n");

    // print state
    cpu_registers_print(&regs);
    fprintf(stdout, "\n");
    mem_print(&mem, 0, 64);

    return 0;
}

