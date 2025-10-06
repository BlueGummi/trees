#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct splay_node {
    uint64_t key;
    struct splay_node *left;
    struct splay_node *right;
    struct splay_node *parent;
};

struct splay_tree {
    struct splay_node *root;
};

void rotate_left(struct splay_tree *tree, struct splay_node *x) {
    struct splay_node *y = x->right;
    x->right = y->left;
    if (y->left)
        y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void rotate_right(struct splay_tree *tree, struct splay_node *y) {
    struct splay_node *x = y->left;
    y->left = x->right;
    if (x->right)
        x->right->parent = y;
    x->parent = y->parent;
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    x->right = y;
    y->parent = x;
}

void splay(struct splay_tree *tree, struct splay_node *x) {
    while (x->parent) {
        if (!x->parent->parent) {
            /* Zig */
            if (x->parent->left == x)
                rotate_right(tree, x->parent);
            else
                rotate_left(tree, x->parent);
        } else if (x->parent->left == x && x->parent->parent->left == x->parent) {
            /* Zig-Zig (left-left) */
            rotate_right(tree, x->parent->parent);
            rotate_right(tree, x->parent);
        } else if (x->parent->right == x && x->parent->parent->right == x->parent) {
            /* Zig-Zig (right-right) */
            rotate_left(tree, x->parent->parent);
            rotate_left(tree, x->parent);
        } else if (x->parent->right == x && x->parent->parent->left == x->parent) {
            /* Zig-Zag (left-right) */
            rotate_left(tree, x->parent);
            rotate_right(tree, x->parent);
        } else {
            /* Zig-Zag (right-left) */
            rotate_right(tree, x->parent);
            rotate_left(tree, x->parent);
        }
    }
}

static int splay_verify_node(struct splay_node *node, uint64_t *min, uint64_t *max) {
    if (!node)
        return 1;

    if (node->left) {
        assert(node->left->parent == node);
        assert(node->left->key < node->key);
        assert(splay_verify_node(node->left, min, &node->key));
    } else if (min)
        assert(!(*min >= node->key));

    if (node->right) {
        assert(node->right->parent == node);
        assert(node->right->key > node->key);
        assert(splay_verify_node(node->right, &node->key, max));
    } else if (max)
        assert(!(*max <= node->key));

    return 1;
}

void splay_verify(struct splay_tree *tree) {
    if (!tree->root)
        return;
    assert(tree->root->parent == NULL);
    splay_verify_node(tree->root, NULL, NULL);
}

struct splay_node *splay_search(struct splay_tree *tree, uint64_t key) {
    struct splay_node *x = tree->root;
    struct splay_node *last = NULL;

    while (x) {
        last = x;
        if (key == x->key)
            break;
        else if (key < x->key)
            x = x->left;
        else
            x = x->right;
    }

    if (last)
        splay(tree, last);

    if (x && x->key == key)
        return x;
    else
        return NULL;
}

void splay_insert(struct splay_tree *tree, uint64_t key) {
    struct splay_node *z = tree->root;
    struct splay_node *p = NULL;

    while (z) {
        p = z;
        if (key < z->key)
            z = z->left;
        else if (key > z->key)
            z = z->right;
        else {
            splay(tree, z);
            return;
        }
    }

    struct splay_node *n = malloc(sizeof(*n));
    n->key = key;
    n->left = n->right = NULL;
    n->parent = p;

    if (!p)
        tree->root = n;
    else if (key < p->key)
        p->left = n;
    else
        p->right = n;

    splay(tree, n);
}

void splay_delete(struct splay_tree *tree, uint64_t key) {
    struct splay_node *node = splay_search(tree, key);
    if (!node || node->key != key)
        return;

    /* Node now at root */
    if (!node->left) {
        tree->root = node->right;
        if (tree->root)
            tree->root->parent = NULL;
    } else {
        struct splay_node *left_subtree = node->left;
        left_subtree->parent = NULL;

        /* find max in left subtree */
        struct splay_node *max = left_subtree;
        while (max->right)
            max = max->right;

        splay(tree, max);

        /* attach right subtree */
        max->right = node->right;
        if (max->right)
            max->right->parent = max;
        tree->root = max;
    }

    free(node);
}

void export_splay_dot(FILE *fp, struct splay_node *node) {
    if (!node)
        return;
    fprintf(fp, "    \"%llu\" [label=\"%llu\"];\n", node->key, node->key);
    if (node->left) {
        fprintf(fp, "    \"%llu\" -> \"%llu\";\n", node->key, node->left->key);
        export_splay_dot(fp, node->left);
    } else {
        fprintf(fp, "    \"nullL%llu\" [shape=circle, label=\"\"];\n", node->key);
        fprintf(fp, "    \"%llu\" -> \"nullL%llu\";\n", node->key, node->key);
    }
    if (node->right) {
        fprintf(fp, "    \"%llu\" -> \"%llu\";\n", node->key, node->right->key);
        export_splay_dot(fp, node->right);
    } else {
        fprintf(fp, "    \"nullR%llu\" [shape=circle, label=\"\"];\n", node->key);
        fprintf(fp, "    \"%llu\" -> \"nullR%llu\";\n", node->key, node->key);
    }
}

void splay_tree_free(struct splay_node *root) {
    if (!root)
        return;
    splay_tree_free(root->left);
    splay_tree_free(root->right);
    free(root);
}

void export_splay_tree_to_dot(struct splay_tree *tree, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        return;
    }
    fprintf(fp, "digraph SplayTree {\n");
    fprintf(fp, "    node [shape=circle, fontname=Arial, fixedsize=true, width=0.7];\n");
    fprintf(fp, "    edge [arrowsize=0.7];\n");
    if (tree->root)
        export_splay_dot(fp, tree->root);
    fprintf(fp, "}\n");
    fclose(fp);
}

#ifndef NUM_INSERTS
#define NUM_INSERTS 100
#endif

#define NUM_REMOVES (NUM_INSERTS / 2)

int main() {
    printf("Splay tree... ");
    fflush(stdout);

    struct splay_tree *tree = malloc(sizeof(struct splay_tree));
    tree->root = NULL;
    int *values = malloc(NUM_INSERTS * sizeof(int));

    srand((unsigned) time(NULL));

    for (int i = 0; i < NUM_INSERTS;) {
        int value = rand() % NUM_INSERTS;
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
        splay_insert(tree, value);
        splay_verify(tree);
    }

    for (int i = NUM_INSERTS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }

    for (int i = 0; i < NUM_REMOVES; i++) {
        splay_delete(tree, values[i]);
        splay_verify(tree);
    }

    for (int i = 0; i < NUM_REMOVES; i++) {
        int idx = rand() % NUM_INSERTS;
        splay_search(tree, values[idx]);
    }

    export_splay_tree_to_dot(tree, "splaytree.dot");

    printf("complete\n");

    splay_tree_free(tree->root);
    free(tree);
    free(values);

    return 0;
}
