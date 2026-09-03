#include "cart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cart_parse_header(struct cart *cart) {
    memcpy(cart->title, &cart->rom[0x134], 16);
    cart->title[16] = '\0';
    cart->type = cart->rom[0x147];
    cart->rom_size = cart->rom[0x148];
    cart->ram_size = cart->rom[0x149];

    // mbc1 unsupported for now!!!
    if (cart->type != 0x00) {
        fprintf(stderr,
                "cart header parse: cart type other than 0x00 not supported - "
                "%02X\n",
                cart->type);
        return -1;
    }
    // check if claimed size is even valid
    if (cart->rom_size > 0x08) {
        fprintf(stderr, "cart header parse: unknown rom size - %02X\n",
                cart->rom_size);
        return -1;
    }
    // check if claimed rom size == real size
    size_t claimed = (size_t)0x8000 << cart->rom_size;
    if (claimed != cart->size) {
        fprintf(stderr, "cart header parse: header says %zu B, file is %zu B\n",
                claimed, cart->size);
        return -1;
    }

    return 0;
}

int cart_load(struct cart *cart, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size < 0) {
        perror("ftell");
        fclose(fp);
        return -1;
    }

    // check if it can hold a header
    if (size < 0x150) {
        fprintf(stderr, "invalid cart: got %ld - too small\n", size);
        fclose(fp);
        return -1;
    }

    rewind(fp);

    uint8_t *rom = malloc((size_t)size);
    if (rom == NULL) {
        perror("malloc");
        fclose(fp);
        return -1;
    }

    size_t read = fread(rom, 1, (size_t)size, fp);
    fclose(fp);

    if (read != (size_t)size) {
        fprintf(stderr, "short read: got %zu of %ld bytes\n", read, size);
        free(rom);
        return -1;
    }

    cart->rom = rom;
    cart->size = (size_t)size;

    if (cart_parse_header(cart) < 0) {
        free(rom);
        return -1;
    }

    // TODO: handle eram and banking

    return 0;
}

void cart_free(struct cart *cart) {
    if (cart == NULL)
        return;
    free(cart->rom);
    cart->rom = NULL;
    free(cart);
}

void cart_print(const struct cart *cart) {
    printf("+----------------------------------------+\n");
    printf("| Cartridge Info                         |\n");
    printf("+----------------------------------------+\n");
    printf("  Title      : %s\n", cart->title);
    printf("  Type       : 0x%02X\n", cart->type);
    printf("  ROM Size   : 0x%02X\n", cart->rom_size);
    printf("  RAM Size   : 0x%02X\n", cart->ram_size);
    printf("  Total Size : %zu bytes (%.1f KiB)\n", cart->size,
           cart->size / 1024.0);
    printf("+----------------------------------------+\n");
}
