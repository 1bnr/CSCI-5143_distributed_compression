
#include "huffman.h"

#include <stdio.h>
#include <string.h>

void print_byte(uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        if ((b << i) & 0x80) {
            printf("1");
        } else {
            printf("0");
        }
    }

    printf(" (%-3d %c) ", b, b);
}

void print_arr(uint8_t *arr, int arrsize)
{
    int cnt = 1;
    printf("// ");
    for (int i = 0; i < arrsize; ++i) {
        print_byte(arr[i]);

        if (cnt % 4 == 0) {
            printf("\n// ");
        }

        cnt++;
    }
}

int main() {
    const char *data = "ABCDDFDDDDDDFEEEEE";
    const uint16_t data_size = (uint16_t) strlen(data);

    uint8_t out_data[64];
    memset(&out_data[0], 0, 64);

    huffman_compress((uint8_t *) data, data_size, &out_data[0]);

    uint8_t decompressed_data[64];
    memset(&decompressed_data[0], 0, 64);

    huffman_inflate(&out_data[0], data_size, &decompressed_data[0]);
    printf("// %s\n", decompressed_data);

    print_arr(&out_data[0], 64);
    return 0;
}