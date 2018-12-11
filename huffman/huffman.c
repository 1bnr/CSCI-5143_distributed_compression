

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

static uint8_t count_distinct_chars(const uint8_t *data, uint16_t data_size)
{
    uint32_t counted_chars[8] = {0}; // space for 32 * 8 = 256 bits

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

static void serialize_tree(node_t *tree, uint8_t *out_data)
{
    if (tree->left) {
        serialize_tree(tree->left, out_data);
        out_data++;
    }

    if (tree->right) {
        serialize_tree(tree->right, out_data);
        out_data++;
    }

    if (tree->byte == 0) {
        // internal node
        *out_data = '*';
    } else {
        *out_data = tree->byte;
    }
}

void bst_print_dot(node_t *tree, FILE *stream);

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
    }

    bst_print_dot(tree, stdout);
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
        fprintf(stream, ";\n");
        bst_print_dot_aux(node->left, stream);
    }

    if (node->right)
    {
        fprintf(stream, "    ");
        bst_print_byte(node, stream);
        fprintf(stream, " -> ");
        bst_print_byte(node->right, stream);
        fprintf(stream, ";\n");
        bst_print_dot_aux(node->right, stream);
    }
}

void bst_print_dot(node_t* tree, FILE* stream)
{
    fprintf(stream, "digraph BST {\n");
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
