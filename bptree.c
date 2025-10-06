#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BPTREE_ORDER 16
#define BPTREE_MAX_KEYS(order) ((order) - 1)
#define BPTREE_MIN_KEYS(order) (((order) + 1) / 2 - 1)

struct bptree_node {
    bool leaf;
    int32_t num_keys;
    int32_t keys[BPTREE_ORDER];
    struct bptree_node *children[BPTREE_ORDER + 1];
    struct bptree_node *next, *prev; /* Leaves only */
    int dot_id;
};

struct bptree {
    struct bptree_node *root;
    int32_t order;
};

struct bptree *bptree_create(int32_t order) {
    struct bptree *tree = malloc(sizeof(*tree));
    tree->order = order;

    struct bptree_node *root = calloc(1, sizeof(*root));
    root->leaf = true;

    tree->root = root;
    return tree;
}

void *bptree_search(struct bptree *tree, int32_t key) {
    struct bptree_node *node = tree->root;

    while (!node->leaf) {
        int32_t i = 0;
        while (i < node->num_keys && key >= node->keys[i])
            i++;
        node = node->children[i];
    }

    for (int32_t i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key)
            return node->children[i];
    }

    return NULL;
}

static struct bptree_node *
bptree_split_leaf(struct bptree *tree, struct bptree_node *leaf, int32_t *promoted_key) {
    int mid = leaf->num_keys / 2; /* floor(n/2) keys in left */
    int right_count = leaf->num_keys - mid;

    struct bptree_node *new_leaf = calloc(1, sizeof(*new_leaf));
    new_leaf->leaf = true;

    /* Copy second half keys/children */
    memcpy(new_leaf->keys, leaf->keys + mid, right_count * sizeof(int32_t));
    memcpy(new_leaf->children, leaf->children + mid, right_count * sizeof(void *));
    new_leaf->num_keys = right_count;
    leaf->num_keys = mid;

    /* Fix leaf chain */
    new_leaf->next = leaf->next;
    new_leaf->prev = leaf;
    if (leaf->next)
        leaf->next->prev = new_leaf;
    leaf->next = new_leaf;

    /* Promote first key of the new leaf */
    *promoted_key = new_leaf->keys[0];

    return new_leaf;
}

static struct bptree_node *
bptree_insert_internal(struct bptree *tree, struct bptree_node *node,
                       int32_t key, void *value, int32_t *promoted_key) {
    int i = 0;
    while (i < node->num_keys && key >= node->keys[i])
        i++;

    if (node->leaf) {
        /* Shift keys/children right to make space */
        for (int j = node->num_keys; j > i; j--) {
            node->keys[j] = node->keys[j - 1];
            node->children[j] = node->children[j - 1];
        }

        node->keys[i] = key;
        node->children[i] = value;
        node->num_keys++;
    } else {
        int32_t child_promoted;
        struct bptree_node *new_child =
            bptree_insert_internal(tree, node->children[i], key, value, &child_promoted);

        if (new_child) {
            /* Shift keys/children to make space in parent */
            for (int j = node->num_keys; j > i; j--) {
                node->keys[j] = node->keys[j - 1];
                node->children[j + 1] = node->children[j];
            }

            node->keys[i] = child_promoted;
            node->children[i + 1] = new_child;
            node->num_keys++;
        }
    }

    /* Split if node overflows */
    if (node->num_keys >= tree->order) {
        if (node->leaf) {
            return bptree_split_leaf(tree, node, promoted_key);
        } else {
            int32_t mid = node->num_keys / 2;
            int32_t right_count = node->num_keys - mid - 1;

            struct bptree_node *new_node = calloc(1, sizeof(*new_node));
            new_node->leaf = false;

            /* Copy keys/children to new internal node */
            memcpy(new_node->keys, node->keys + mid + 1, right_count * sizeof(int32_t));
            memcpy(new_node->children, node->children + mid + 1, (right_count + 1) * sizeof(void *));
            new_node->num_keys = right_count;

            *promoted_key = node->keys[mid];
            node->num_keys = mid;

            return new_node;
        }
    }

    return NULL;
}

void bptree_insert(struct bptree *tree, int32_t key, void *value) {
    int32_t promoted;
    struct bptree_node *new_node =
        bptree_insert_internal(tree, tree->root, key, value, &promoted);

    if (new_node) {
        struct bptree_node *new_root = calloc(1, sizeof(*new_root));
        new_root->keys[0] = promoted;
        new_root->children[0] = tree->root;
        new_root->children[1] = new_node;
        new_root->num_keys = 1;
        tree->root = new_root;
    }
}

bool bptree_verify_leaf_chain(struct bptree *tree) {
    struct bptree_node *node = tree->root;
    while (!node->leaf)
        node = node->children[0];

    int32_t prev_key = -2147483648;
    while (node) {
        for (int32_t i = 0; i < node->num_keys; i++) {
            if (node->keys[i] < prev_key) {
                fprintf(stderr, "Leaf chain out of order: %d < %d\n",
                        node->keys[i], prev_key);
                return false;
            }
            prev_key = node->keys[i];
        }
        node = node->next;
    }
    return true;
}

static int32_t bptree_verify_node(struct bptree_node *node, int32_t order, int32_t depth, int32_t *leaf_depth) {
    for (int32_t i = 1; i < node->num_keys; i++) {
        if (node->keys[i - 1] >= node->keys[i]) {
            fprintf(stderr, "Key order violation: %d >= %d at depth %d\n",
                    node->keys[i - 1], node->keys[i], depth);
            return 0;
        }
    }

    if (node->num_keys < 0 || node->num_keys > order) {
        fprintf(stderr, "Invalid key count %d (max %d) at depth %d\n",
                node->num_keys, order, depth);
        return 0;
    }

    if (node->leaf) {
        if (*leaf_depth == -1)
            *leaf_depth = depth;
        else if (*leaf_depth != depth) {
            fprintf(stderr, "Leaf depth mismatch at depth %d (expected %d)\n",
                    depth, *leaf_depth);
            return 0;
        }
        return 1;
    }

    for (int32_t i = 0; i <= node->num_keys; i++) {
        if (!node->children[i]) {
            fprintf(stderr, "Null child in internal node at depth %d\n", depth);
            return 0;
        }

        if (!bptree_verify_node(node->children[i], order, depth + 1, leaf_depth))
            return 0;

        if (i > 0) {
            struct bptree_node *left = node->children[i - 1];
            struct bptree_node *right = node->children[i];

            int32_t left_max = left->keys[left->num_keys - 1];
            int32_t right_min = right->keys[0];
            int32_t this_key = node->keys[i - 1];

            if (!(left_max <= this_key && this_key <= right_min)) {
                fprintf(stderr,
                        "Key range violation at depth %d: left_max=%d key=%d right_min=%d\n",
                        depth, left_max, this_key, right_min);
                return 0;
            }
        }
    }

    return 1;
}

bool bptree_verify(struct bptree *tree) {
    if (!tree->root)
        return true;

    int32_t leaf_depth = -1;
    bptree_verify_leaf_chain(tree);
    return bptree_verify_node(tree->root, tree->order, 0, &leaf_depth);
}

static void borrow_from_left(struct bptree *tree, struct bptree_node *parent, int idx) {
    struct bptree_node *child = parent->children[idx];
    struct bptree_node *left = parent->children[idx - 1];

    if (child->leaf) {
        /* Shift child’s keys right to make space */
        for (int i = child->num_keys; i > 0; i--) {
            child->keys[i] = child->keys[i - 1];
            child->children[i] = child->children[i - 1];
        }

        /* Move last key/value from left sibling to child */
        child->keys[0] = left->keys[left->num_keys - 1];
        child->children[0] = left->children[left->num_keys - 1];

        /* Update parent separator */
        parent->keys[idx - 1] = child->keys[0];

        left->num_keys--;
        child->num_keys++;
    } else {
        /* Internal node */
        for (int i = child->num_keys; i > 0; i--) {
            child->keys[i] = child->keys[i - 1];
            child->children[i + 1] = child->children[i];
        }
        child->children[1] = child->children[0];

        child->keys[0] = parent->keys[idx - 1];
        child->children[0] = left->children[left->num_keys];

        parent->keys[idx - 1] = left->keys[left->num_keys - 1];

        left->num_keys--;
        child->num_keys++;
    }
}

static void borrow_from_right(struct bptree *tree, struct bptree_node *parent, int idx) {
    struct bptree_node *child = parent->children[idx];
    struct bptree_node *right = parent->children[idx + 1];

    if (child->leaf) {
        child->keys[child->num_keys] = right->keys[0];
        child->children[child->num_keys] = right->children[0];
        child->num_keys++;

        /* Shift right sibling left */
        for (int i = 0; i < right->num_keys - 1; i++) {
            right->keys[i] = right->keys[i + 1];
            right->children[i] = right->children[i + 1];
        }
        right->num_keys--;

        /* Update parent separator */
        parent->keys[idx] = right->keys[0];
    } else {
        child->keys[child->num_keys] = parent->keys[idx];
        child->children[child->num_keys + 1] = right->children[0];

        parent->keys[idx] = right->keys[0];

        /* Shift right sibling */
        for (int i = 0; i < right->num_keys - 1; i++) {
            right->keys[i] = right->keys[i + 1];
            right->children[i] = right->children[i + 1];
        }
        right->children[right->num_keys - 1] = right->children[right->num_keys];
        right->num_keys--;

        child->num_keys++;
    }
}

static void merge_nodes(struct bptree *tree, struct bptree_node *parent, int idx) {
    struct bptree_node *left = parent->children[idx];
    struct bptree_node *right = parent->children[idx + 1];

    if (left->leaf) {
        /* Merge keys/values */
        memcpy(left->keys + left->num_keys, right->keys, right->num_keys * sizeof(int32_t));
        memcpy(left->children + left->num_keys, right->children, right->num_keys * sizeof(void *));
        left->num_keys += right->num_keys;

        /* Fix leaf chain */
        left->next = right->next;
        if (right->next)
            right->next->prev = left;
    } else {
        /* Internal node merge */
        left->keys[left->num_keys] = parent->keys[idx];
        memcpy(left->keys + left->num_keys + 1, right->keys, right->num_keys * sizeof(int32_t));
        memcpy(left->children + left->num_keys + 1, right->children, (right->num_keys + 1) * sizeof(void *));
        left->num_keys += right->num_keys + 1;
    }

    /* Remove separator key from parent */
    for (int i = idx; i < parent->num_keys - 1; i++) {
        parent->keys[i] = parent->keys[i + 1];
        parent->children[i + 1] = parent->children[i + 2];
    }
    parent->num_keys--;

    free(right);
}

/* Helper: update parent keys if the first key in a child changed */
static void update_parent_key(struct bptree_node *root, struct bptree_node *child) {
    if (!root || root == child)
        return;

    struct bptree_node *node = root;
    while (!node->leaf) {
        int i = 0;
        while (i <= node->num_keys && node->children[i] != child)
            i++;
        if (i > node->num_keys)
            return; // child not in this path

        if (i > 0)
            node->keys[i - 1] = child->keys[0];

        node = node->children[i];
    }
}

/* Remove key from a leaf node */
static void remove_from_leaf(struct bptree *tree, struct bptree_node *leaf, int idx) {
    for (int i = idx; i < leaf->num_keys - 1; i++) {
        leaf->keys[i] = leaf->keys[i + 1];
        leaf->children[i] = leaf->children[i + 1];
    }
    leaf->num_keys--;

    /* Update parent keys if first key changed */
    if (idx == 0)
        update_parent_key(tree->root, leaf);
}

/* Fix underflow of child at index idx in parent */
static void fix_underflow(struct bptree *tree, struct bptree_node *parent, int idx) {
    struct bptree_node *left = (idx > 0) ? parent->children[idx - 1] : NULL;
    struct bptree_node *right = (idx < parent->num_keys) ? parent->children[idx + 1] : NULL;

    if (left && left->num_keys > BPTREE_MIN_KEYS(tree->order)) {
        borrow_from_left(tree, parent, idx);
    } else if (right && right->num_keys > BPTREE_MIN_KEYS(tree->order)) {
        borrow_from_right(tree, parent, idx);
    } else if (left) {
        merge_nodes(tree, parent, idx - 1);
    } else if (right) {
        merge_nodes(tree, parent, idx);
    }
}

static inline int find_index(struct bptree_node *node, int32_t key) {
    int i = 0;
    while (i < node->num_keys && key >= node->keys[i])
        i++;
    return i;
}

static bool delete_recursive(struct bptree *tree, struct bptree_node *node, int32_t key) {
    if (node->leaf) {
        int idx = 0;
        while (idx < node->num_keys && node->keys[idx] != key)
            idx++;

        if (idx == node->num_keys) {
            printf("recursive deletion failed for %d\n", key);
            return false;
        }

        remove_from_leaf(tree, node, idx);
        return true;
    }

    int idx = find_index(node, key);
    struct bptree_node *child = node->children[idx];
    bool deleted = delete_recursive(tree, child, key);

    if (child->num_keys < BPTREE_MIN_KEYS(tree->order))
        fix_underflow(tree, node, idx);

    return deleted;
}

bool bptree_delete(struct bptree *tree, int32_t key) {
    if (!tree->root)
        return false;

    bool deleted = delete_recursive(tree, tree->root, key);

    /* If root has no keys, promote first child */
    if (tree->root->num_keys == 0 && !tree->root->leaf) {
        struct bptree_node *old_root = tree->root;
        tree->root = tree->root->children[0];
        free(old_root);
    }

    return deleted;
}

static void bptree_dot_node(FILE *f, struct bptree_node *node, int *id) {
    node->dot_id = (*id)++;
    int my_id = node->dot_id;

    if (node->leaf) {
        fprintf(f, "  node%d [label=\"", my_id);
        for (int i = 0; i < node->num_keys; i++) {
            fprintf(f, "%d", node->keys[i]);
            if (i != node->num_keys - 1)
                fprintf(f, " | ");
        }
        fprintf(f, "\", shape=box, style=filled, color=lightgray];\n");
    } else {
        fprintf(f, "  node%d [label=\"", my_id);
        for (int i = 0; i < node->num_keys; i++) {
            fprintf(f, "<f%d> | %d |", i, node->keys[i]);
        }
        fprintf(f, "<f%d>\"];\n", node->num_keys);

        for (int i = 0; i <= node->num_keys; i++) {
            bptree_dot_node(f, node->children[i], id);
            fprintf(f, "  node%d:f%d -> node%d;\n", my_id, i, node->children[i]->dot_id);
        }
    }
}

static void bptree_dot_leaves(FILE *f, struct bptree_node *root) {
    struct bptree_node *node = root;
    while (!node->leaf)
        node = node->children[0];

    fprintf(f, "  { rank=same; ");
    while (node) {
        fprintf(f, "node%d; ", node->dot_id);
        node = node->next;
    }
    fprintf(f, "}\n");

    node = root;
    while (!node->leaf)
        node = node->children[0];

    while (node && node->next) {
        fprintf(f, "  node%d -> node%d [style=dashed, color=blue];\n",
                node->dot_id, node->next->dot_id);
        node = node->next;
    }
}

void export_bptree_to_dot(struct bptree *tree, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f)
        return;

    fprintf(f, "digraph BPTree {\n");
    fprintf(f, "  node [shape=record];\n");

    int id = 0;
    bptree_dot_node(f, tree->root, &id);
    bptree_dot_leaves(f, tree->root);

    fprintf(f, "}\n");
    fclose(f);
}

#define NUM_INSERTS 100
#define NUM_REMOVES 50

int main(void) {
    printf("B+ tree... ");
    fflush(stdout);

    struct bptree *tree = bptree_create(BPTREE_ORDER);
    int *values = malloc(NUM_INSERTS * sizeof(int));
    srand((unsigned) time(NULL));

    for (int i = 0; i < NUM_INSERTS;) {
        int value = rand() % (NUM_INSERTS * 2);

        /* avoid duplicates */
        int duplicate = 0;
        for (int j = 0; j < i; j++) {
            if (values[j] == value) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate)
            continue;

        values[i++] = value;
        bptree_insert(tree, value, (void *) (uintptr_t) value);
    }

    /* Verify full tree after inserts */
    assert(bptree_verify(tree));

    for (int i = NUM_INSERTS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }

    for (int i = 0; i < NUM_REMOVES; i++) {
        bool ok = bptree_delete(tree, values[i]);
        if (!ok) {
            fprintf(stderr, "Failed to delete %d\n", values[i]);
            abort();
        }
        if ((i % 500) == 0) {
            assert(bptree_verify(tree));
        }
    }

    assert(bptree_verify(tree));
    assert(bptree_verify_leaf_chain(tree));

    export_bptree_to_dot(tree, "bptree.dot");
    printf("complete\n");

    free(values);
    free(tree);
    return 0;
}
