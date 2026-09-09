#include "cart/cart.h"
#include "cart/mbc0.h"
#include "cart/mbc1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cart_parse_header(struct cart *cart) {
    memcpy(cart->title, &cart->rom[0x134], 16);
    cart->title[16] = '\0';
    cart->type = cart->rom[0x147];
    cart->rom_code = cart->rom[0x148];
    cart->ram_code = cart->rom[0x149];

    // rom
    // check if claimed size is even valid
    if (cart->rom_code > 0x08) {
        fprintf(stderr, "cart header parse: unknown rom size - %02X\n",
                cart->rom_code);
        return -1;
    }
    // check if claimed rom size == real size
    size_t claimed = (size_t)0x8000 << cart->rom_code;
    if (claimed != cart->rom_bytes) {
        fprintf(stderr, "cart header parse: header says %zu B, file is %zu B\n",
                claimed, cart->rom_bytes);
        return -1;
    }

    // ram
    switch (cart->ram_code) {
    case 0x00: // none
    case 0x01: // unused
        break;
    case 0x02: // 8 KiB
        cart->ram_bytes = 8 * 1024;
        break;
    case 0x03: // 32 KiB
        cart->ram_bytes = 32 * 1024;
        break;
    case 0x04: // 128 KiB
        cart->ram_bytes = 128 * 1024;
        break;
    case 0x05: // 64 KiB
        cart->ram_bytes = 64 * 1024;
        break;
    default:
        break;
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

    memset(cart, 0, sizeof(*cart));
    cart->rom = rom;
    cart->rom_bytes = (size_t)size;

    if (cart_parse_header(cart) < 0) {
        free(rom);
        return -1;
    }


    cart->ram = cart->ram_bytes ? calloc(1, cart->ram_bytes) : NULL;

    switch (cart->type) {
    case 0x00: // rom only
        mbc0_init(cart);
        break;
    case 0x01: // mbc1
    case 0x02: // mbc1 + ram
    case 0x03: // mbc1 + ram + battery
        mbc1_init(cart);
        break;
    case 0x05: // mbc2
    case 0x06: // mbc2 + battery
    case 0x08: // rom+ram
    case 0x09: // rom+ram+battery
    case 0x0B: // MMM01
    case 0x0C: // MMM01 + ram
    case 0x0D: // MMM01 + ram + battery
    case 0x0F: // mbc3 + timer + battery
    case 0x10: // mbc3 + timer + ram + battery
    case 0x11: // mbc3
    case 0x12: // mbc3 + ram
    case 0x13: // mbc3 + ram + battery
    case 0x19: // mbc5
    case 0x1A: // mbc5 + ram
    case 0x1B: // mbc5 + ram + battery
    case 0x1C: // mbc5 + rumble
    case 0x1D: // mbc5 + rumble + ram
    case 0x1E: // mbc5 + rumble + ram + ram + battery
    case 0x20: // mbc6
    case 0x22: // mbc7 + sensor + rumble + ram + battery
    case 0xFC: // pocket camera
    case 0xFD: // bandai tamas (the pink gbp is immaculate)
    case 0xFE: // huc3
    case 0xFF: // huc1 + ram + battery
    default:
        fprintf(stderr, "Unsupported cart: type %02X\n", cart->type);
        free(cart->rom);
        free(cart->ram);
        return -1;
    }

    return 0;
}

void cart_free(struct cart *cart) {
    if (cart == NULL)
        return;
    free(cart->rom);
    cart->rom = NULL;
    free(cart->ram);
    cart->ram = NULL;
}

void cart_print(const struct cart *cart) {
    printf("+----------------------------------------+\n");
    printf("| Cartridge Info                         |\n");
    printf("+----------------------------------------+\n");
    printf("  Title      : %s\n", cart->title);
    printf("  Type       : 0x%02X\n", cart->type);
    printf("  ROM Size   : 0x%02X\n", cart->rom_code);
    printf("  RAM Size   : 0x%02X\n", cart->ram_code);
    printf("  Total Size : %zu bytes (%.1f KiB)\n", cart->rom_bytes,
           cart->rom_bytes / 1024.0);
    printf("+----------------------------------------+\n");
}
