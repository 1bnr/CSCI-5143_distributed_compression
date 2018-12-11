#defin DELIN "*"

#include "huffman.h"

#include <stdint.h>
#include <stddef.h>

typedef struct node {
    struct node *left;
    struct node *right;
    uint16_t weight;
    uint8_t byte;
} node_t;

static uint8_t count_distinct_chars(const uint8_t *data, uint16_t data_size) {
    uint32_t counted_chars[8] = { 0 }; // space for 32 * 8 = 256 bits

    for (int i = 0; i < data_size; i++) {
        uint8_t c = data[data_size];
        counted_chars[c / 8] |= 1 << (c % 32);
    }

    uint8_t total_distinct_chars = 0;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 32; j++) {
            total_distinct_chars += (counted_chars[i] & (1 << j)) ? 1 : 0;
        }
    }

    return total_distinct_chars;
}

static void build_histogram(node_t *histogram, const uint8_t *data, uint16_t data_size) {
    int hist_index = 0;

    for (int i = 0; i < data_size; i++) {
        uint8_t byte = data[i];

        int found = 0;

        for (int j = 0; j < hist_index; ++j) {
            if (histogram[j].byte == byte) {
                histogram[j].weight++;
                found = 1;
            }
        }

        if (!found) {
            histogram[hist_index].byte = byte;
            histogram[hist_index].weight = 1;
            hist_index++;
        }
    }
}

static uint16_t calc_tree_size(uint8_t nr_of_distinct_characters) {
    return (uint16_t)((uint16_t)nr_of_distinct_characters * 2 - 1);
}

static void find_smallest_two_nodes(node_t *tree, uint16_t tree_size, node_t **a, node_t **b) {
    *a = NULL;
    *b = NULL;

    for (int i = 0; i < tree_size; i++) {
        if (tree[i].weight < (*a)->weight) {
            *b = *a;
            *a = &tree[i];
        }
    }
}

static void build_tree(node_t *tree, uint16_t tree_size) {
    uint16_t lower_index = 0;
    uint16_t tree_index = tree_size;

    while (hist_index < histogram_size) {
        node_t *a, *b;
        find_smallest_two_nodes(&tree[lower_index], tree_size, &a, &b);

        node_t *internal_node = &tree[tree_index++];
        internal_node->weight = a->weight + b->weight;
        internal_node->left = a;
        internal_node->right = b;
        internal_node->byte = 0;
        tree_size++;
    }
}

// uint8_t s_tree is an array 2*tree_size -1, zerowrite prior to passing in

static void serialize_tree(node_t *tree, uint16_t tree_size, uint8_t *s_tree) {
    if (tree) {
        for (int i = 0; i < sizeof(s_tree), i++) {
            if (s_tree[i]) {
                s_tree[i] = DELIN;
                break;
            }
        }
    } else { // at a leaf, recird byte
        for (int i = 0; i < sizeof(s_tree), i++) {
            if (s_tree[i]) {
                s_tree[i] = tree->byte;
                break;
            }
        }
        serialize_tree(tree->left, tree_size, s_tree);
        serialize_tree(tree->right, tree_size, s_tree);
    }
}
