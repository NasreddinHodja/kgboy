#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>
#include <stdint.h>

// 16 bit addr -> 2^16 bytes
#define MEM_SIZE 0x10000

struct memory {
    uint8_t data[MEM_SIZE];
};

void mem_init(struct memory *mem);
uint8_t mem_read(struct memory *mem, uint16_t addr);
void mem_write(struct memory *mem, uint16_t addr, uint8_t val);
int mem_load_rom(struct memory *mem, const char *path);

int mem_print(struct memory *mem, uint16_t start, size_t length);

#endif // MEMORY_H_
