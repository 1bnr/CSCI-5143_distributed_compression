
#include "huffman.h"

#include <stdio.h>
#include <string.h>

void print_arr(uint8_t *arr, int arrsize)
{
    for (int i = 0; i < arrsize; ++i) {
        printf("%c ", arr[i]);
    }
}

int main() {
    const char *data = "AABBCCDDDFFOOQQQGGGGGGGGGGGGGGGGGGGGGGGGGPPPPPPDDDLLLRRRRRRRR";
    const uint16_t data_size = (uint16_t) strlen(data);

    uint8_t out_data[16];

    huffman_compress((uint8_t *) data, data_size, &out_data[0]);

    return 0;
}