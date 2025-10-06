#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum red_black_tree_node_color { TREE_NODE_RED,
                                 TREE_NODE_BLACK };

struct red_black_tree_node {
    int data;
    enum red_black_tree_node_color color;
    struct red_black_tree_node *left;
    struct red_black_tree_node *right;
    struct red_black_tree_node *parent;
};

struct red_black_tree {
    struct red_black_tree_node *root;
};

struct red_black_tree *red_black_tree_create(void) {
    struct red_black_tree *tree = (struct red_black_tree *) malloc(sizeof(struct red_black_tree));
    tree->root = NULL;
    return tree;
}

struct red_black_tree_node *tree_find_min(struct red_black_tree_node *node) {
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

void rb_transplant(struct red_black_tree *tree, struct red_black_tree_node *u, struct red_black_tree_node *v) {
    if (u->parent == NULL)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;

    if (v)
        v->parent = u->parent;
}

void left_rotate(struct red_black_tree *tree, struct red_black_tree_node *x) {
    struct red_black_tree_node *y = x->right;
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

void right_rotate(struct red_black_tree *tree, struct red_black_tree_node *y) {
    struct red_black_tree_node *x = y->left;
    y->left = x->right;
    if (x->right)
        x->right->parent = y;

    x->parent = y->parent;
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;

    x->right = y;
    y->parent = x;
}

void fix_deletion(struct red_black_tree *tree, struct red_black_tree_node *x) {
    while (x != tree->root && (!x || x->color == TREE_NODE_BLACK)) {
        if (!x || !x->parent)
            break;

        struct red_black_tree_node *sibling;

        if (x == x->parent->left) {
            sibling = x->parent->right;

            if (sibling && sibling->color == TREE_NODE_RED) {
                sibling->color = TREE_NODE_BLACK;
                x->parent->color = TREE_NODE_RED;
                left_rotate(tree, x->parent);
                sibling = x->parent->right;
            }

            if (!sibling ||
                ((!sibling->left || sibling->left->color == TREE_NODE_BLACK) &&
                 (!sibling->right || sibling->right->color == TREE_NODE_BLACK))) {
                if (sibling)
                    sibling->color = TREE_NODE_RED;
                x = x->parent;
            } else {
                if (!sibling->right || sibling->right->color == TREE_NODE_BLACK) {
                    if (sibling->left)
                        sibling->left->color = TREE_NODE_BLACK;
                    if (sibling) {
                        sibling->color = TREE_NODE_RED;
                        right_rotate(tree, sibling);
                        sibling = x->parent->right;
                    }
                }

                if (sibling) {
                    sibling->color = x->parent->color;
                    x->parent->color = TREE_NODE_BLACK;
                    if (sibling->right)
                        sibling->right->color = TREE_NODE_BLACK;
                    left_rotate(tree, x->parent);
                }
                x = tree->root;
            }
        } else {
            sibling = x->parent->left;

            if (sibling && sibling->color == TREE_NODE_RED) {
                sibling->color = TREE_NODE_BLACK;
                x->parent->color = TREE_NODE_RED;
                right_rotate(tree, x->parent);
                sibling = x->parent->left;
            }

            if (!sibling ||
                ((!sibling->left || sibling->left->color == TREE_NODE_BLACK) &&
                 (!sibling->right || sibling->right->color == TREE_NODE_BLACK))) {
                if (sibling)
                    sibling->color = TREE_NODE_RED;
                x = x->parent;
            } else {
                if (!sibling->left || sibling->left->color == TREE_NODE_BLACK) {
                    if (sibling->right)
                        sibling->right->color = TREE_NODE_BLACK;
                    if (sibling) {
                        sibling->color = TREE_NODE_RED;
                        left_rotate(tree, sibling);
                        sibling = x->parent->left;
                    }
                }

                if (sibling) {
                    sibling->color = x->parent->color;
                    x->parent->color = TREE_NODE_BLACK;
                    if (sibling->left)
                        sibling->left->color = TREE_NODE_BLACK;
                    right_rotate(tree, x->parent);
                }
                x = tree->root;
            }
        }
    }

    if (x)
        x->color = TREE_NODE_BLACK;
}

int validate_rbtree(struct red_black_tree_node *node, int *black_height) {
    if (node == NULL) {
        *black_height = 1;
        return 1;
    }

    if (node->color == TREE_NODE_RED) {
        if ((node->left && node->left->color == TREE_NODE_RED) ||
            (node->right && node->right->color == TREE_NODE_RED)) {
            fprintf(stderr, "Red-Red violation at node %d\n", node->data);
            return 0;
        }
    }

    int left_black_height = 0;
    int right_black_height = 0;

    if (!validate_rbtree(node->left, &left_black_height))
        return 0;
    if (!validate_rbtree(node->right, &right_black_height))
        return 0;

    if (left_black_height != right_black_height) {
        fprintf(stderr, "Black-height violation at node %d (left height=%d, right height=%d)\n",
                node->data, left_black_height, right_black_height);
        return 0;
    }

    *black_height = left_black_height + (node->color == TREE_NODE_BLACK ? 1 : 0);
    return 1;
}

void rb_delete(struct red_black_tree *tree, struct red_black_tree_node *z) {
    struct red_black_tree_node *y = z;
    struct red_black_tree_node *x = NULL;
    enum red_black_tree_node_color y_original_color = y->color;

    if (z->left == NULL) {
        x = z->right;
        rb_transplant(tree, z, z->right);
    } else if (z->right == NULL) {
        x = z->left;
        rb_transplant(tree, z, z->left);
    } else {
        y = tree_find_min(z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent != z) {
            rb_transplant(tree, y, y->right);
            y->right = z->right;
            if (y->right)
                y->right->parent = y;
        }

        rb_transplant(tree, z, y);
        y->left = z->left;
        if (y->left)
            y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == TREE_NODE_BLACK) {
        fix_deletion(tree, x);
    }

    free(z);
}

struct red_black_tree_node *tree_search(struct red_black_tree_node *root, int data) {
    while (root && root->data != data) {
        if (data < root->data)
            root = root->left;
        else
            root = root->right;
    }
    return root;
}

void red_black_tree_remove(struct red_black_tree *tree, int data) {
    struct red_black_tree_node *node = tree_search(tree->root, data);
    if (node)
        rb_delete(tree, node);
}

void red_black_tree_free(struct red_black_tree_node *root) {
    if (root == NULL)
        return;
    red_black_tree_free(root->left);
    red_black_tree_free(root->right);
    root->left = root->right = NULL;
    free(root);
}

void fix_insertion(struct red_black_tree *tree, struct red_black_tree_node *node) {
    while (node != tree->root && node->parent->color == TREE_NODE_RED) {
        struct red_black_tree_node *parent = node->parent;
        struct red_black_tree_node *grandparent = parent->parent;

        if (parent == grandparent->left) {
            struct red_black_tree_node *uncle = grandparent->right;
            if (uncle && uncle->color == TREE_NODE_RED) {
                parent->color = TREE_NODE_BLACK;
                uncle->color = TREE_NODE_BLACK;
                grandparent->color = TREE_NODE_RED;
                node = grandparent;
            } else {
                if (node == parent->right) {
                    node = parent;
                    left_rotate(tree, node);
                    parent = node->parent;
                }
                parent->color = TREE_NODE_BLACK;
                grandparent->color = TREE_NODE_RED;
                right_rotate(tree, grandparent);
            }
        } else {
            struct red_black_tree_node *uncle = grandparent->left;
            if (uncle && uncle->color == TREE_NODE_RED) {
                parent->color = TREE_NODE_BLACK;
                uncle->color = TREE_NODE_BLACK;
                grandparent->color = TREE_NODE_RED;
                node = grandparent;
            } else {
                if (node == parent->left) {
                    node = parent;
                    right_rotate(tree, node);
                    parent = node->parent;
                }
                parent->color = TREE_NODE_BLACK;
                grandparent->color = TREE_NODE_RED;
                left_rotate(tree, grandparent);
            }
        }
    }
    tree->root->color = TREE_NODE_BLACK;
}

void red_black_tree_insert(struct red_black_tree *tree, int data) {
    struct red_black_tree_node *new_node = malloc(sizeof(struct red_black_tree_node));
    new_node->data = data;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->color = TREE_NODE_RED;

    if (tree->root == NULL) {
        new_node->color = TREE_NODE_BLACK;
        new_node->parent = NULL;
        tree->root = new_node;
        return;
    }

    struct red_black_tree_node *current = tree->root;
    struct red_black_tree_node *parent = NULL;
    while (current != NULL) {
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

    fix_insertion(tree, new_node);

    if (parent)
        assert(!(parent->color == TREE_NODE_RED && new_node->color == TREE_NODE_RED));

    int dummy_bh = 0;
    assert(validate_rbtree(tree->root, &dummy_bh));
}

#define ANSI_RED "\033[31m"
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"

#define RBT_RED ANSI_RED "R" ANSI_RESET
#define RBT_BLACK ANSI_BOLD "B" ANSI_RESET

void print_inorder(struct red_black_tree_node *node) {
    if (node == NULL)
        return;
    print_inorder(node->left);
    printf("(%d,%s) ", node->data, node->color == TREE_NODE_RED ? RBT_RED : RBT_BLACK);
    print_inorder(node->right);
}
void export_dot(FILE *fp, struct red_black_tree_node *node) {
    if (!node)
        return;

    fprintf(fp, "    \"%d\" [label=\"%d\", color=%s, fontcolor=%s, style=filled, fillcolor=%s];\n",
            node->data,
            node->data,
            node->color == TREE_NODE_RED ? "\"red\"" : "\"gray\"",
            "white",
            node->color == TREE_NODE_RED ? "\"#ffcccc\"" : "\"#808080\"");

    if (node->left) {
        fprintf(fp, "    \"%d\" -> \"%d\";\n", node->data, node->left->data);
        export_dot(fp, node->left);
    } else {
        fprintf(fp, "    \"nullL%d\" [shape=circle, label=\"\", fontcolor=\"black\"];\n", node->data);
        fprintf(fp, "    \"%d\" -> \"nullL%d\";\n", node->data, node->data);
    }

    if (node->right) {
        fprintf(fp, "    \"%d\" -> \"%d\";\n", node->data, node->right->data);
        export_dot(fp, node->right);
    } else {
        fprintf(fp, "    \"nullR%d\" [shape=circle, label=\"\", fontcolor=\"black\"];\n", node->data);
        fprintf(fp, "    \"%d\" -> \"nullR%d\";\n", node->data, node->data);
    }
}

void export_tree_to_dot(struct red_black_tree *tree, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        return;
    }

    fprintf(fp, "digraph RedBlackTree {\n");
    fprintf(fp, "    node [shape=circle, fontname=Arial, fixedsize=true, width=0.7];\n");
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
    printf("Red-black tree... ");
    fflush(stdout);
    struct red_black_tree *tree = red_black_tree_create();
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
        red_black_tree_insert(tree, value);
    }

    for (int i = NUM_INSERTS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }

    for (int i = 0; i < NUM_REMOVES; i++) {
        red_black_tree_remove(tree, values[i]);
    }

    export_tree_to_dot(tree, "rbtree.dot");
    printf("complete\n");

    red_black_tree_free(tree->root);
    free(tree);
    free(values);

    return 0;
}
