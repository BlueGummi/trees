#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct avl_tree_node {
    int data;
    int height;
    struct avl_tree_node *left;
    struct avl_tree_node *right;
    struct avl_tree_node *parent;
};

struct avl_tree {
    struct avl_tree_node *root;
};

static inline int height(struct avl_tree_node *node) {
    return node ? node->height : 0;
}

static inline int balance_factor(struct avl_tree_node *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

static void update_height(struct avl_tree_node *node) {
    if (node)
        node->height =
            1 + (height(node->left) > height(node->right) ? height(node->left)
                                                          : height(node->right));
}

void left_rotate(struct avl_tree *tree, struct avl_tree_node *x) {
    struct avl_tree_node *y = x->right;
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

    update_height(x);
    update_height(y);
}

void right_rotate(struct avl_tree *tree, struct avl_tree_node *y) {
    struct avl_tree_node *x = y->left;
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

    update_height(y);
    update_height(x);
}

void rebalance(struct avl_tree *tree, struct avl_tree_node *node) {
    while (node) {
        update_height(node);
        int bf = balance_factor(node);

        if (bf > 1) {
            if (balance_factor(node->left) < 0)
                left_rotate(tree, node->left);
            right_rotate(tree, node);
        } else if (bf < -1) {
            if (balance_factor(node->right) > 0)
                right_rotate(tree, node->right);
            left_rotate(tree, node);
        }

        node = node->parent;
    }
}

void avl_tree_insert(struct avl_tree *tree, int data) {
    struct avl_tree_node *new_node = malloc(sizeof(struct avl_tree_node));
    new_node->data = data;
    new_node->left = new_node->right = NULL;
    new_node->height = 1;

    if (!tree->root) {
        new_node->parent = NULL;
        tree->root = new_node;
        return;
    }

    struct avl_tree_node *current = tree->root;
    struct avl_tree_node *parent = NULL;
    while (current) {
        parent = current;
        if (data < current->data)
            current = current->left;
        else
            current = current->right;
    }

    new_node->parent = parent;
    if (data < parent->data)
        parent->left = new_node;
    else
        parent->right = new_node;

    rebalance(tree, parent);
}

static struct avl_tree_node *
min_node(struct avl_tree_node *node) {
    while (node && node->left)
        node = node->left;
    return node;
}

static void
transplant(struct avl_tree *tree,
           struct avl_tree_node *u,
           struct avl_tree_node *v) {
    if (!u->parent)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

void avl_tree_remove(struct avl_tree *tree, int data) {
    struct avl_tree_node *node = tree->root;
    while (node && node->data != data)
        node = (data < node->data) ? node->left : node->right;
    if (!node)
        return;

    struct avl_tree_node *rebalance_start = node->parent;

    if (!node->left) {
        transplant(tree, node, node->right);
    } else if (!node->right) {
        transplant(tree, node, node->left);
    } else {
        struct avl_tree_node *succ = min_node(node->right);
        struct avl_tree_node *rebalance_from = succ->parent;

        if (succ->parent != node) {
            transplant(tree, succ, succ->right);
            succ->right = node->right;
            if (succ->right)
                succ->right->parent = succ;
        } else {
            rebalance_from = succ;
        }

        transplant(tree, node, succ);
        succ->left = node->left;
        if (succ->left)
            succ->left->parent = succ;

        update_height(succ);

        rebalance_start = rebalance_from;
    }

    free(node);
    if (rebalance_start)
        rebalance(tree, rebalance_start);
}

int validate_avltree(struct avl_tree_node *node, int *height_out) {
    if (!node) {
        *height_out = 0;
        return 1;
    }

    int lh = 0, rh = 0;
    if (!validate_avltree(node->left, &lh))
        return 0;
    if (!validate_avltree(node->right, &rh))
        return 0;

    int bf = lh - rh;
    if (bf < -1 || bf > 1) {
        fprintf(stderr, "AVL balance violation at %d (bf = %d)\n", node->data, bf);
        return 0;
    }

    *height_out = 1 + (lh > rh ? lh : rh);
    return 1;
}

void avl_tree_free(struct avl_tree_node *root) {
    if (root == NULL)
        return;

    avl_tree_free(root->left);
    avl_tree_free(root->right);
    root->left = root->right = NULL;
    free(root);
}

#define ANSI_RED "\033[31m"
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"

#define RBT_RED ANSI_RED "R" ANSI_RESET
#define RBT_BLACK ANSI_BOLD "B" ANSI_RESET

void print_inorder(struct avl_tree_node *node) {
    if (node == NULL)
        return;
    print_inorder(node->left);
    printf("(%d) ", node->data);
    print_inorder(node->right);
}

void export_dot(FILE *fp, struct avl_tree_node *node) {
    if (!node)
        return;

    fprintf(fp,
            "    \"%d\" [label=\"%d\", color=\"gray\", fontcolor=\"white\", style=filled, "
            "fillcolor=\"#808080\"];\n",
            node->data, node->data);

    if (node->left) {
        fprintf(fp, "    \"%d\" -> \"%d\";\n", node->data, node->left->data);
        export_dot(fp, node->left);
    } else {
        fprintf(
            fp,
            "    \"nullL%d\" [shape=circle, label=\"\", fontcolor=\"black\"];\n",
            node->data);
        fprintf(fp, "    \"%d\" -> \"nullL%d\";\n", node->data, node->data);
    }

    if (node->right) {
        fprintf(fp, "    \"%d\" -> \"%d\";\n", node->data, node->right->data);
        export_dot(fp, node->right);
    } else {
        fprintf(
            fp,
            "    \"nullR%d\" [shape=circle, label=\"\", fontcolor=\"black\"];\n",
            node->data);
        fprintf(fp, "    \"%d\" -> \"nullR%d\";\n", node->data, node->data);
    }
}

void export_tree_to_dot(struct avl_tree *tree, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        return;
    }

    fprintf(fp, "digraph RedBlackTree {\n");
    fprintf(
        fp,
        "    node [shape=circle, fontname=Arial, fixedsize=true, width=0.7];\n");
    fprintf(fp, "    edge [arrowsize=0.7];\n");

    if (tree->root)
        export_dot(fp, tree->root);

    fprintf(fp, "}\n");
    fclose(fp);
}

#ifndef NUM_INSERTS
#define NUM_INSERTS 100
#endif

#define NUM_REMOVES NUM_INSERTS / 2

int main() {
    printf("AVL tree... ");
    fflush(stdout);
    struct avl_tree *tree = malloc(sizeof(struct avl_tree));
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
        avl_tree_insert(tree, value);
    }

    for (int i = NUM_INSERTS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }

    for (int i = 0; i < NUM_REMOVES; i++) {
        avl_tree_remove(tree, values[i]);
        int h;
        assert(validate_avltree(tree->root, &h));
    }

    export_tree_to_dot(tree, "avltree.dot");
    printf("complete\n");

    avl_tree_free(tree->root);
    free(tree);
    free(values);

    return 0;
}
