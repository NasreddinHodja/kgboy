#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// TODO: we will have to move these eventually,
// or think better about how to keep these definitions
// so we don't have magic numbers
#define ROM_BANK_00 0x0000
#define ROM_BANK_SIZE (16 * 1024)

void mem_init(struct memory *mem) {
    memset(mem->data, 0, MEM_SIZE);
}

uint8_t mem_read(struct memory *mem, uint16_t addr) {
    return mem->data[addr];
}
void mem_write(struct memory *mem, uint16_t addr, uint8_t val) {
    mem->data[addr] = val;
}

int mem_load_rom(struct memory *mem, const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    // 1. load rom bank 00
    // 0000 to 3FFF -> 16KiB
    
    fread(mem->data, 1, ROM_BANK_SIZE, file);

    // 2. TODO: loand bank 01-N
    // (we will just ignore this for now)

    fclose(file);

    return 0;
}

int mem_print(struct memory *mem, uint16_t start, size_t length) {
    if (start + length > MEM_SIZE) {
        fprintf(stderr, "Memory out of bounds\n");
        return -1;
    } 

    for (size_t i = 0; i < (length + 15) / 16; i++) {
        // print addr for line
        fprintf(stdout, "%04X:  ",
                (uint16_t) (start + (16 * i)));

        // print data in line
        for (size_t j = 0; j < 16 && ((i * 16) + j) < length; j++) {
            fprintf(stdout, "%02X ",
                    mem->data[start + (i * 16) + j]);
            if ((j + 1) % 4 == 0) fprintf(stdout, " ");
            if ((j + 1) % 8 == 0) fprintf(stdout, " ");
        }
        fprintf(stdout, "\n");
    }

    return 0;
}
