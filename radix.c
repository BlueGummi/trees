#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RADIX_BITS 6
#define RADIX_SIZE (1 << RADIX_BITS)
#define RADIX_MASK (RADIX_SIZE - 1)

#define NUM_INSERTS 128
#define NUM_LOOKUPS 32

struct radix_node {
    struct radix_node *parent;
    struct radix_node *slots[RADIX_SIZE];
    uint64_t key_part;
    uint64_t present_mask;
};

struct radix_tree {
    struct radix_node *root;
    uint32_t height;
};

static inline uint64_t radix_index(uint64_t key, uint32_t level) {
    uint32_t shift = level * RADIX_BITS;
    return (key >> shift) & RADIX_MASK;
}

int32_t radix_insert(struct radix_tree *tree, uint64_t key, struct radix_node *new_node) {
    int32_t level = tree->height;

    if (!tree->root) {
        tree->root = calloc(1, sizeof(struct radix_node));
        tree->root->key_part = 0;
    }

    struct radix_node *node = tree->root;

    for (; level > 1; level--) {
        uint64_t idx = radix_index(key, level - 1);

        if (!node->slots[idx]) {
            struct radix_node *mid = calloc(1, sizeof(struct radix_node));
            mid->key_part = idx;
            mid->parent = node;
            node->slots[idx] = mid;
            node->present_mask |= (1ULL << idx);
        }

        node = node->slots[idx];
    }

    uint64_t idx = radix_index(key, 0);
    if (node->slots[idx])
        return -EEXIST;

    node->slots[idx] = new_node;
    new_node->parent = node;
    node->present_mask |= (1ULL << idx);

    return 0;
}

struct radix_node *radix_lookup(struct radix_tree *tree, uint64_t key) {
    struct radix_node *node = tree->root;
    for (int32_t level = tree->height; level > 0; level--) {
        if (!node)
            return NULL;
        uint64_t idx = radix_index(key, level - 1);
        node = node->slots[idx];
    }
    return node;
}

static bool radix_verify_node(struct radix_node *node,
                              struct radix_node *expected_parent,
                              int level, int max_height,
                              int *node_count) {
    if (!node)
        return true;

    if (node->parent != expected_parent) {
        fprintf(stderr, "Node %p has incorrect parent %p (expected %p)\n",
                (void *) node, (void *) node->parent, (void *) expected_parent);
        return false;
    }

    if (node_count)
        (*node_count)++;

    uint64_t expected_mask = 0;
    for (int i = 0; i < RADIX_SIZE; i++) {
        if (node->slots[i])
            expected_mask |= (1ULL << i);
    }

    if (expected_mask != node->present_mask) {
        fprintf(stderr, "Node %p present_mask mismatch: expected 0x%llx, got 0x%llx\n",
                (void *) node, (unsigned long long) expected_mask,
                (unsigned long long) node->present_mask);
        return false;
    }

    for (int i = 0; i < RADIX_SIZE; i++) {
        struct radix_node *child = node->slots[i];

        if (level > max_height && child) {
            fprintf(stderr, "Node %p at level %d has child beyond max height %d\n",
                    (void *) node, level, max_height);
            return false;
        }

        if (child && !radix_verify_node(child, node, level + 1, max_height, node_count))
            return false;
    }

    return true;
}

bool radix_verify_tree(struct radix_tree *tree) {
    if (!tree)
        return true;

    if (!tree->root) {
        if (tree->height != 0)
            fprintf(stderr, "Tree has no root but nonzero height %u\n", tree->height);
        return (tree->height == 0);
    }

    int node_count = 0;
    return radix_verify_node(tree->root, NULL, 0, tree->height, &node_count);
}

static void radix_prune_up(struct radix_node *node, struct radix_tree *tree) {
    while (node && node->present_mask == 0) {
        struct radix_node *parent = node->parent;
        if (!parent) {
            if (tree->root == node) {
                free(node);
                tree->root = NULL;
            }
            break;
        }

        for (uint64_t i = 0; i < RADIX_SIZE; i++) {
            if (parent->slots[i] == node) {
                parent->slots[i] = NULL;
                parent->present_mask &= ~(1ULL << i);
                break;
            }
        }

        free(node);
        node = parent;
    }
}

int32_t radix_delete(struct radix_tree *tree, uint64_t key) {
    struct radix_node *node = tree->root;
    struct radix_node *parent = NULL;
    uint64_t idx = 0;

    for (int32_t level = tree->height; level > 0; level--) {
        if (!node)
            return -ENOENT;

        parent = node;
        idx = radix_index(key, level - 1);
        node = node->slots[idx];
    }

    if (!node)
        return -ENOENT;

    free(node);
    parent->slots[idx] = NULL;
    parent->present_mask &= ~(1ULL << idx);

    radix_prune_up(parent, tree);

    return 0;
}

static void export_radix_dot(FILE *fp, struct radix_node *node, int level) {
    if (!node)
        return;

    fprintf(fp,
            "    \"%p\" [label=\"%llu (L%d)\", shape=box, style=filled, fillcolor=\"#808080\", fontcolor=\"white\"];\n",
            (void *) node, node->key_part, level);

    for (int i = 0; i < RADIX_SIZE; i++) {
        struct radix_node *child = node->slots[i];
        if (child) {
            fprintf(fp, "    \"%p\" -> \"%p\" [label=\"%d\"];\n",
                    (void *) node, (void *) child, i);
            export_radix_dot(fp, child, level + 1);
        }
    }
}

void export_radix_tree_to_dot(struct radix_tree *tree, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        return;
    }

    fprintf(fp, "digraph RadixTree {\n");
    fprintf(fp, "    node [fontname=Arial];\n");

    if (tree->root)
        export_radix_dot(fp, tree->root, 0);

    fprintf(fp, "}\n");
    fclose(fp);
}

struct radix_node *radix_create_node(uint64_t key_part) {
    struct radix_node *node = calloc(1, sizeof(struct radix_node));
    node->key_part = key_part;
    return node;
}

static void radix_free_node(struct radix_node *node) {
    if (!node)
        return;

    /* Recursively free all children */
    for (int i = 0; i < RADIX_SIZE; i++)
        if (node->slots[i])
            radix_free_node(node->slots[i]);

    free(node);
}

void radix_free_tree(struct radix_tree *tree) {
    if (!tree)
        return;

    if (tree->root)
        radix_free_node(tree->root);

    tree->root = NULL;
    tree->height = 0;
}

int main(void) {
    printf("Radix tree ... ");
    fflush(stdout);

    struct radix_tree tree = {0};
    tree.height = 3;

    struct radix_node *root = radix_create_node(0);
    tree.root = root;

    srand((unsigned) time(NULL));
    uint64_t *keys = malloc(NUM_INSERTS * sizeof(uint64_t));

    for (int i = 0; i < NUM_INSERTS;) {
        uint64_t key = rand() % 4096;
        int duplicate = 0;
        for (int j = 0; j < i; j++) {
            if (keys[j] == key) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate)
            continue;

        struct radix_node *n = radix_create_node(key);
        int ret = radix_insert(&tree, key, n);
        assert(radix_verify_tree(&tree));
        if (ret != 0 && ret != -EEXIST)
            fprintf(stderr, "Insert failed for key %llu: %d\n", key, ret);

        keys[i++] = key;
    }

    for (int i = 0; i < NUM_LOOKUPS; i++) {
        uint64_t key = keys[rand() % NUM_INSERTS];
        struct radix_node *found = radix_lookup(&tree, key);
        assert(radix_verify_tree(&tree));
        assert(found && "lookup failed for an inserted key");
    }

    for (int i = 0; i < NUM_INSERTS / 2; i++) {
        int ret = radix_delete(&tree, keys[i]);
        assert(radix_verify_tree(&tree));
        if (ret != 0)
            fprintf(stderr, "Delete failed for key %llu: %d\n", keys[i], ret);
    }

    export_radix_tree_to_dot(&tree, "radixtree.dot");
    printf("complete\n");

    radix_free_tree(&tree);
    free(keys);
    return 0;
}
