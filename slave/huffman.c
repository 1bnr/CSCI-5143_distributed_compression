

#include "huffman.h"

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>

// 7 bytes
typedef struct node {
    struct node *left;
    struct node *right;
    uint16_t weight;
    uint8_t byte;
} node_t;

// 4 bytes
typedef struct queue {
    struct queue *next;
    node_t *el;
} queue_t;

typedef struct encoding_info {
    uint8_t byte;
    uint8_t encoded_byte;
    uint8_t num_bits;
} encoding_info_t;

typedef struct {
    uint8_t *buff;
    uint16_t buff_index;

    uint16_t bitnum;
} bitstream_t;

#define ALPHABET_SIZE (26 + 26 + 7) // a-zA-Z?.,;[CR][LF][Space]
#define TREE_MAX_SIZE (ALPHABET_SIZE * 2 - 1)
#define QUEUE_MAX_SIZE (ALPHABET_SIZE)

typedef struct {
    node_t *tree_memory;
    node_t *tree_ptr;
    uint16_t allocated_tree_size;

    queue_t *queue_memory;
    queue_t *queue_ptr;
    uint16_t allocated_queue_size;
} huffman_t;

static node_t *tree_alloc(huffman_t *h, uint16_t weight)
{
    node_t *n = h->tree_ptr++;
    n->left = NULL;
    n->right = NULL;
    n->weight = weight;
    n->byte = 0;
    h->allocated_tree_size++;
    return n;
}

static queue_t *queue_alloc(huffman_t *h, node_t *el)
{
    queue_t *n = h->queue_ptr++;
    n->next = NULL;
    n->el = el;
    h->allocated_queue_size++;
    return n;
}

static void queue_clear(huffman_t *h)
{
    h->queue_ptr = h->queue_memory;
}

static queue_t *queue_add_element(huffman_t *h, queue_t *head, node_t *el)
{
    queue_t *curr = head;

    queue_t *new = queue_alloc(h, el);

    if (curr == NULL) {
        return new;
    }

    if (curr->el->weight > el->weight) {
        new->next = curr;
        return new;
    }

    while (curr->next != NULL) {
        if (curr->next->el->weight > el->weight) {
            new->next = curr->next;
            curr->next = new;
            return head;
        }

        curr = curr->next;
    }

    curr->next = new;
    return head;
}

static queue_t *queue_remove_element(queue_t *head, node_t *el)
{
    queue_t *curr = head;
    queue_t *prev = NULL;

    while (curr != NULL) {
        if (el == curr->el) {
            if (prev == NULL) {
                return curr; // remove head
            } else {
                prev->next = curr->next;
                return head;
            }
        }

        prev = curr;
        curr = head->next;
    }

    return head;
}

static queue_t *queue_pop2(queue_t *head, node_t **a, node_t **b)
{
    *a = head->el;
    *b = head->next ? head->next->el : NULL;

    if (head->next) {
        return head->next->next;
    }

    return NULL;
}

static uint16_t queue_size(queue_t *head)
{
    uint16_t size = 0;

    queue_t *curr = head;

    while (curr != NULL) {
        size++;
        curr = curr->next;
    }

    return size;
}

static queue_t *build_queue(huffman_t *h, const uint8_t *data, uint16_t data_size)
{
    queue_t *head = NULL;

    for (int i = 0; i < data_size; i++) {
        uint8_t byte = data[i];

        int found = 0;
        queue_t *curr = head;

        while (curr != NULL) {
            if (curr->el->byte == byte) {
                curr->el->weight++;
                found = 1;
                break;
            }

            curr = curr->next;
        }

        if (!found) {
            node_t *new_node = tree_alloc(h, 1);
            new_node->byte = byte;

            head = queue_add_element(h, head, new_node);
        }
    }

    return head;
}

static node_t *build_tree(huffman_t *h, queue_t *head)
{
    while (queue_size(head) >= 2) {
        node_t *a, *b;
        head = queue_pop2(head, &a, &b);

        node_t *internal_node = tree_alloc(h, a->weight + b->weight);
        internal_node->left = a;
        internal_node->right = b;
        internal_node->byte = 0;

        head = queue_add_element(h, head, internal_node);
    }

    // queue now contains only 1 node
    assert(head != NULL);
    node_t *root = head->el;

    return root;
}

static int tree_get_size(node_t *tree)
{
    int size = 0;

    if (tree->left) {
        size += tree_get_size(tree->left);
    }

    if (tree->right) {
        size += tree_get_size(tree->right);
    }

    return size + 1;
}

static uint16_t tree_count_leaves(node_t *tree)
{
    if (tree->byte != 0) {
        return 1;
    }

    uint16_t sz = 0;
    if (tree->left) {
        sz += tree_count_leaves(tree->left);
    }

    if (tree->right) {
        sz += tree_count_leaves(tree->right);
    }

    return sz;
}

static void serialize_tree_node(node_t *tree, uint8_t **out_data)
{
    if (tree == NULL) {
        (**out_data) = '-';
        (*out_data)++;
        return;
    }

    if (tree->byte != 0) {
        // leaf node
        (**out_data) = tree->byte;
        (*out_data)++;
        return;
    }

    //internal node
    (**out_data) = '*';
    (*out_data)++;

    serialize_tree_node(tree->left, out_data);
    serialize_tree_node(tree->right, out_data);
}

static void serialize_tree(node_t *tree, uint8_t *data)
{
    serialize_tree_node(tree, &data);
}

static void build_encoding_table(node_t *node, uint8_t encoded_byte, uint8_t num_bits, encoding_info_t **info_arr)
{
    if (node->byte != 0) {
        (**info_arr).byte = node->byte;
        (**info_arr).encoded_byte = encoded_byte;
        (**info_arr).num_bits = num_bits;
        (*info_arr)++;
        return;
    }

    if (node->left) {
        build_encoding_table(node->left, (uint8_t) ((encoded_byte << 1) | 0), (uint8_t) (num_bits + 1), info_arr);
    }

    if (node->right) {
        build_encoding_table(node->right, (uint8_t) ((encoded_byte << 1) | 1), (uint8_t) (num_bits + 1), info_arr);
    }
}

static uint8_t find_encoded_byte(encoding_info_t *info_arr, uint16_t info_arr_size, uint8_t b, uint8_t *out_num_bits)
{
    for (int i = 0; i < info_arr_size; ++i) {
        if (info_arr[i].byte == b) {
            *out_num_bits = info_arr[i].num_bits;
            return info_arr[i].encoded_byte;
        }
    }

    // char not in encoding table - skip
    *out_num_bits = 0;
    return 0;
}

static void bitstream_put_bit(bitstream_t *bitstream, uint8_t bit)
{
    uint8_t *current_byte = &bitstream->buff[bitstream->buff_index];

    bit &= 1;
    *current_byte = (bit << bitstream->bitnum) | *current_byte;

    bitstream->bitnum++;

    if (bitstream->bitnum == 8) {
        bitstream->bitnum = 0;
        bitstream->buff_index++;
    }
}

static void bitstream_put_bits(bitstream_t *bitstream, uint8_t byte, uint8_t num_bits)
{
    for (int i = 0; i < num_bits; i++) {
        bitstream_put_bit(bitstream, (uint8_t) (byte & 1));
        byte >>= 1;
    }
}


static uint8_t reverse_bits(uint8_t b, uint8_t num_bits)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return (uint8_t) (b >> (8 - num_bits));
}

static uint16_t compress_data(encoding_info_t *info_arr, uint16_t info_arr_size, const uint8_t *data, uint16_t data_size, uint8_t *out_data)
{
    uint16_t total_bits = 0;

    bitstream_t bitstream = {.buff_index = 0, .buff = out_data, .bitnum = 0};

    for (uint16_t i = 0; i < data_size; i++) {
        uint8_t b = data[i];

        uint8_t num_bits;
        uint8_t encoded_byte = find_encoded_byte(info_arr, info_arr_size, b, &num_bits);
        encoded_byte = reverse_bits(encoded_byte, num_bits);
        bitstream_put_bits(&bitstream, encoded_byte, num_bits);

        total_bits += num_bits;
    }

    return total_bits;
}

void bst_print_dot(const char *name, node_t *tree, FILE *stream);

void huffman_compress(uint8_t *data, uint16_t data_size, uint8_t *out_data)
{
    node_t tree_memory[TREE_MAX_SIZE];
    huffman_t huffman_memory = {
            .tree_memory = &tree_memory[0],
            .tree_ptr = &tree_memory[0]
    };

    node_t *tree;
    {
        // we want to free the queue memory asap
        // therefore we open a new scope
        queue_t queue_memory[QUEUE_MAX_SIZE];
        huffman_memory.queue_ptr = &queue_memory[0];
        huffman_memory.queue_memory = &queue_memory[0];
        queue_t *queue_head = build_queue(&huffman_memory, data, data_size);
        tree = build_tree(&huffman_memory, queue_head);
//        bst_print_dot("compress", tree, stdout);
    }

    int tree_sz = tree_get_size(tree);

    out_data[0] = (uint8_t) tree_sz;
    serialize_tree(tree, out_data + 3);

    uint16_t encoding_table_size = tree_count_leaves(tree);
    encoding_info_t encoding_table[encoding_table_size];
    encoding_info_t *table_ptr = &encoding_table[0];
    build_encoding_table(tree, 0, 0, &table_ptr);

    uint16_t num_bits = compress_data(&encoding_table[0], encoding_table_size, data, data_size, out_data + 3 + tree_sz);

    out_data[1] = (uint8_t) ((num_bits >> 8) & 0xFF);
    out_data[2] = (uint8_t) (num_bits & 0xFF);
}

static node_t *deserialize_tree_node(huffman_t *h, uint8_t **data_ptr)
{
    if (**data_ptr == '-') {
        (*data_ptr)++;
        return NULL;
    }

    if (**data_ptr != '*') {
        node_t *node = tree_alloc(h, 0);
        node->byte = **data_ptr;
        (*data_ptr)++;
        return node;
    }

    node_t *internal_node = tree_alloc(h, 0);
    internal_node->byte = 0;
    (*data_ptr)++;

    internal_node->left = deserialize_tree_node(h, data_ptr);
    internal_node->right = deserialize_tree_node(h, data_ptr);

    return internal_node;
}

static node_t *deserialize_tree(huffman_t *h, uint8_t *data, uint16_t *out_bytes_read)
{
    uint8_t *data_ptr_cpy = data;
    node_t *tree = deserialize_tree_node(h, &data_ptr_cpy);
    *out_bytes_read = (uint16_t) (data_ptr_cpy - data);
    return tree;
}

static uint8_t bitstream_read_bit(bitstream_t *bitstream)
{
    uint8_t bit = bitstream->buff[bitstream->buff_index] >> bitstream->bitnum;
    bit &= 1;

    bitstream->bitnum++;

    if (bitstream->bitnum == 8) {
        bitstream->bitnum = 0;
        bitstream->buff_index++;
    }

    return bit;
}

static uint8_t deserialize_next_char(node_t *tree, bitstream_t *bitstream, uint8_t *num_bits)
{
    node_t *curr = tree;

    while (1) {
        uint8_t next_bit = bitstream_read_bit(bitstream);
        (*num_bits)++;

        if (next_bit == 0) {
            curr = curr->left;
        } else {
            curr = curr->right;
        }

        if (curr == NULL) {
            return '?';
        }

        if (curr->byte != 0) {
            // leaf
            return curr->byte;
        }
    }
}

static void inflate_data(node_t *tree, uint8_t *data, uint16_t data_size, uint16_t total_bits, uint8_t *out_data)
{
    uint16_t bits_read = 0;

    bitstream_t bitstream = {.buff = &data[0], .buff_index = 0, .bitnum = 0};

    while (bits_read < total_bits && bitstream.buff_index < data_size) {
        uint8_t num_bits_read = 0;
        uint8_t next_char = deserialize_next_char(tree, &bitstream, &num_bits_read);

        *(out_data++) = next_char;

        bits_read += num_bits_read;
    }
}

void huffman_inflate(uint8_t *data, uint16_t data_size, uint8_t *out_data)
{
    uint16_t tree_size = data[0];
    uint16_t num_bits = (data[1] << 8) | data[2];

    node_t tree_memory[tree_size];
    huffman_t huffman_memory = {
            .tree_memory = &tree_memory[0],
            .tree_ptr = &tree_memory[0],
    };

    uint16_t deserialized_tree_bytes_read;
    node_t *tree = deserialize_tree(&huffman_memory, data + 3, &deserialized_tree_bytes_read);

    uint16_t encoding_table_size = tree_count_leaves(tree);
    encoding_info_t encoding_table[encoding_table_size];
    encoding_info_t *table_ptr = &encoding_table[0];
    build_encoding_table(tree, 0, 0, &table_ptr);

    inflate_data(tree, data + 3 + deserialized_tree_bytes_read, data_size, num_bits, out_data);
}

/*
 * Adapted from https://eli.thegreenplace.net/2009/11/23/visualizing-binary-trees-with-graphviz
 */
void bst_print_dot_null(uint8_t key, int nullcount, FILE* stream)
{
    fprintf(stream, "    null%d [shape=point];\n", nullcount);
    fprintf(stream, "    %c -> null%d;\n", key, nullcount);
}

void bst_print_byte(node_t *node, FILE *stream)
{
    static int internal_node_count = 1;
    if (node->byte == 0) {
        fprintf(stream, "\"I%p %d\"", node, node->weight);
    } else {
        fprintf(stream, "\"%c %d\"", node->byte, node->weight);
    }
}

void bst_print_dot_aux(node_t* node, FILE* stream)
{
    if (node->left)
    {
        fprintf(stream, "    ");
        bst_print_byte(node, stream);
        fprintf(stream, " -> ");
        bst_print_byte(node->left, stream);
        fprintf(stream, " [ label=\"0\" ];\n");
        bst_print_dot_aux(node->left, stream);
    }

    if (node->right)
    {
        fprintf(stream, "    ");
        bst_print_byte(node, stream);
        fprintf(stream, " -> ");
        bst_print_byte(node->right, stream);
        fprintf(stream, " [ label=\"1\" ];\n");
        bst_print_dot_aux(node->right, stream);
    }
}

void bst_print_dot(const char *name, node_t* tree, FILE* stream)
{
    fprintf(stream, "digraph %s {\n", name);
    fprintf(stream, "    node [fontname=\"Arial\"];\n");

    if (!tree)
        fprintf(stream, "\n");
    else if (!tree->right && !tree->left) {
        fprintf(stream, "    ");
        bst_print_byte(tree, stream);
        fprintf(stream, ";\n");
    } else
        bst_print_dot_aux(tree, stream);

    fprintf(stream, "}\n");
}
