#ifndef GB_H_
#define GB_H_

#include "cpu.h"
#include "memory.h"

struct gb {
    struct cpu_registers regs;
    struct memory mem;
};

#endif // GB_H_
