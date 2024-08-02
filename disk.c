#include "disk.h"

int get_byt_per_sec(char * p) {
    return *(unsigned short*) (p + 11);
}

int read_FAT_entry(char * p, int sector) {
    char * first_byte = p + byt_per_sec + (3 * sector) / 2;
    char * second_byte = first_byte + 1;
    if (sector % 2 == 0) {
        // for even entries, concatenate low nibble of second byte with first byte
        return ((*(unsigned short*) first_byte) & 0xff) \
                + (((*(unsigned short*) second_byte) & 0xf) << 8);
    } else {
        // for odd entries, concatenate second byte with high nibble of first byte
        return (((*(unsigned short*) second_byte) & 0xff) << 4) \
                + (((*(unsigned short*) first_byte) & 0xf0) >> 4);
    }
}

int calculate_free_space(char * p) {
    int free_space = 0;
    for (int offset = 2; offset < 2880; offset ++) {
        int value = read_FAT_entry(p, offset);
        if (value == 0) {
            free_space ++;
        }
    }
    return free_space;
}

int filename_compare(char * s1, char * s2, int len) {
    if (s2[0] == 0 || s2[0] == 0x20) {
        return -1;
    }
    for (int i = 0; i < len; i ++) {
        if (s2[i] == 0x20 || s2[i] == 0 || s2[i] == s1[i]) {
            continue;
        } else {
            return -1;
        }
    }
    return 0;
}

char * alloc_uppercase_string(char * str, int len) {
    char * uppercase = calloc(len, 1);
    strcpy(uppercase, str);
    for (int i = 0; i < strlen(uppercase); i ++) {
        uppercase[i] = toupper((unsigned char) uppercase[i]);
    }
    return uppercase;
}