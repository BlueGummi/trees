#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct treap_node {
    uint64_t key;
    uint32_t priority;
    struct treap_node *left, *right;
};

struct treap {
    struct treap_node *root;
};

static struct treap_node *rotate_right(struct treap_node *y) {
    struct treap_node *x = y->left;
    struct treap_node *T2 = x->right;
    x->right = y;
    y->left = T2;
    return x;
}

static struct treap_node *rotate_left(struct treap_node *x) {
    struct treap_node *y = x->right;
    struct treap_node *T2 = y->left;
    y->left = x;
    x->right = T2;
    return y;
}

static struct treap_node *treap_create_node(uint64_t key) {
    struct treap_node *n = calloc(1, sizeof(*n));
    n->key = key;
    n->priority = rand();
    return n;
}

static struct treap_node *treap_insert_node(struct treap_node *root, uint64_t key) {
    if (!root)
        return treap_create_node(key);

    if (key < root->key) {
        root->left = treap_insert_node(root->left, key);
        if (root->left->priority < root->priority)
            root = rotate_right(root);
    } else if (key > root->key) {
        root->right = treap_insert_node(root->right, key);
        if (root->right->priority < root->priority)
            root = rotate_left(root);
    } else {
        /* duplicate */
        return root;
    }

    return root;
}

void treap_insert(struct treap *t, uint64_t key) {
    t->root = treap_insert_node(t->root, key);
}

static bool treap_verify_node(struct treap_node *node,
                              uint64_t min_key,
                              uint64_t max_key,
                              int *count) {
    if (!node)
        return 1;

    /* BST invariant */
    if (node->key <= min_key || node->key >= max_key) {
        fprintf(stderr, "BST violation at node %llu: not in range (%llu, %llu)\n",
                (unsigned long long) node->key,
                (unsigned long long) min_key,
                (unsigned long long) max_key);
        return false;
    }

    /* Heap invariant */
    if (node->left && node->left->priority < node->priority) {
        fprintf(stderr, "Heap violation (left) at node %llu: parent %u, left %u\n",
                (unsigned long long) node->key,
                node->priority,
                node->left->priority);
        return false;
    }
    if (node->right && node->right->priority < node->priority) {
        fprintf(stderr, "Heap violation (right) at node %llu: parent %u, right %u\n",
                (unsigned long long) node->key,
                node->priority,
                node->right->priority);
        return false;
    }

    (*count)++;

    return treap_verify_node(node->left, min_key, node->key, count) &&
           treap_verify_node(node->right, node->key, max_key, count);
}

bool treap_verify(struct treap *t) {
    int count = 0;
    return treap_verify_node(t->root, 0, UINT64_MAX, &count);
}

static struct treap_node *treap_delete_node(struct treap_node *root, uint64_t key) {
    if (!root)
        return NULL;

    if (key < root->key)
        root->left = treap_delete_node(root->left, key);
    else if (key > root->key)
        root->right = treap_delete_node(root->right, key);
    else {
        /* found */
        if (!root->left && !root->right) {
            free(root);
            return NULL;
        } else if (!root->left)
            root = rotate_left(root);
        else if (!root->right)
            root = rotate_right(root);
        else if (root->left->priority < root->right->priority)
            root = rotate_right(root);
        else
            root = rotate_left(root);

        /* delete again */
        root = treap_delete_node(root, key);
    }

    return root;
}

void treap_delete(struct treap *t, uint64_t key) {
    t->root = treap_delete_node(t->root, key);
}

struct treap_node *treap_lookup(struct treap *t, uint64_t key) {
    struct treap_node *n = t->root;
    while (n) {
        if (key < n->key)
            n = n->left;
        else if (key > n->key)
            n = n->right;
        else
            return n;
    }
    return NULL;
}

static void treap_free_node(struct treap_node *n) {
    if (!n)
        return;
    treap_free_node(n->left);
    treap_free_node(n->right);
    free(n);
}

void treap_free(struct treap *t) {
    treap_free_node(t->root);
    t->root = NULL;
}

static void treap_export_dot_node(FILE *f, struct treap_node *n) {
    if (!n)
        return;

    /* Print current node */
    fprintf(f, "    \"%p\" [label=\"%llu\\n(priority = %u)\"];\n",
            (void *) n, n->key, n->priority);

    /* Link left child */
    if (n->left) {
        fprintf(f, "    \"%p\" -> \"%p\" [label=\"L\"];\n", (void *) n, (void *) n->left);
        treap_export_dot_node(f, n->left);
    }

    /* Link right child */
    if (n->right) {
        fprintf(f, "    \"%p\" -> \"%p\" [label=\"R\"];\n", (void *) n, (void *) n->right);
        treap_export_dot_node(f, n->right);
    }
}

void treap_export_to_dot(struct treap *t, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("fopen");
        return;
    }

    fprintf(f, "digraph treap {\n");
    fprintf(f, "    node [shape=record, style=filled, fillcolor=lightgrey];\n");

    if (t->root)
        treap_export_dot_node(f, t->root);
    else
        fprintf(f, "    null [label=\"empty\"];\n");

    fprintf(f, "}\n");
    fclose(f);
}

#ifndef NUM_INSERTS
#define NUM_INSERTS 100
#endif

#define NUM_REMOVES (NUM_INSERTS / 2)

int main(void) {
    printf("Treap... ");
    fflush(stdout);

    struct treap t = {0};
    int *values = malloc(NUM_INSERTS * sizeof(int));
    srand((unsigned) time(NULL));

    /* Insert random unique keys */
    for (int i = 0; i < NUM_INSERTS;) {
        int value = rand() % (NUM_INSERTS * 10);
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
        treap_insert(&t, value);
    }

    assert(treap_verify(&t));

    /* Shuffle keys before removal */
    for (int i = NUM_INSERTS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }

    /* Delete half of them */
    for (int i = 0; i < NUM_REMOVES; i++) {
        treap_delete(&t, values[i]);
        assert(treap_verify(&t));
    }

    /* Lookup the remaining keys */
    for (int i = NUM_REMOVES; i < NUM_INSERTS; i++) {
        struct treap_node *n = treap_lookup(&t, values[i]);
        assert(n && n->key == (uint64_t) values[i]);
    }

    printf("complete\n");

    treap_export_to_dot(&t, "treap.dot");
    treap_free(&t);
    free(values);
    return 0;
}
