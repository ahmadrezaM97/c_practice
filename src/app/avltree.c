
#include "avltree.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void* find_in_subtree(Node* node, int key) {
  if (node->key == key)
    return node->value;
  if (node->key < key)
    return find_in_subtree(node->rc, key);
  if (node->key > key)
    return find_in_subtree(node->lc, key);
}

void* find_in_tree(Tree* tree, int key) {
  assert(tree != NULL);
  if (tree->root)
    return find_in_subtree(tree->root, key);
  return NULL;
}

int get_height(Node* node) {
  if (node == NULL)
    return 0;
  return node->height;
}

int balance(Node* node) {
  if (node == NULL)
    return 0;
  return get_height(node->rc) - get_height(node->lc);
}

void update_height(Node* node) {
  node->height = MAX(get_height(node->lc), get_height(node->rc)) + 1;
}

void update_height_rec(Node* node) {
  Node* p = node;
  while (p != NULL) {
    update_height(p);
    p = p->parent;
  }
}

void insert_to_left(Node* node, Node* c) {
  assert(node != NULL && c != NULL);
  node->lc = c;
  c->parent = node;
  update_height_rec(c);
}

void insert_to_right(Node* node, Node* c) {
  assert(node != NULL && c != NULL);
  node->rc = c;
  c->parent = node;
  update_height_rec(c);
}

Node* rebalance_node(Node* node);

void insert_subtree(Node* root, Node* n) {
  if (root == NULL || n == NULL)
    return;
  if (root->key < n->key) {
    if (root->rc == NULL) {
      insert_to_right(root, n);
      return;
    }
    insert_subtree(root->rc, n);
    root->rc = rebalance_node(root->rc);
    root->rc->parent = root;
    return;
  }

  if (root->lc == NULL) {
    insert_to_left(root, n);
    return;
  }
  insert_subtree(root->lc, n);
  root->lc = rebalance_node(root->lc);
  root->lc->parent = root;
  return;
}

Node* left_rotate(Node* node) {
  Node* right_child = node->rc;
  assert(right_child != NULL);

  node->rc = right_child->lc;
  if (right_child->lc)
    right_child->lc->parent = node;
  right_child->lc = node;

  right_child->parent = node->parent;
  node->parent = right_child;

  update_height_rec(node);
  update_height_rec(right_child);
  return right_child;
}

Node* right_rotate(Node* node) {
  Node* left_child = node->lc;
  assert(left_child != NULL);

  node->lc = left_child->rc;
  if (left_child->lc)
    left_child->lc->parent = node;
  left_child->rc = node;

  left_child->parent = node->parent;
  node->parent = left_child;

  update_height_rec(node);
  update_height_rec(left_child);
  return left_child;
}

Node* rebalance_node(Node* node) {
  if (!node)
    return node;
  // left heavy
  // right_height - left_height
  if (balance(node) < -1) {
    if (balance(node->lc) <= 0) {
      printf("case 1\n");
      return right_rotate(node);
    }
    printf("case 2\n");
    node->lc = left_rotate(node->lc);
    node->lc->parent = node;

    return right_rotate(node);
  }
  if (balance(node) > 1) {
    if (balance(node->rc) >= 0) {
      printf("case 3\n");
      return left_rotate(node);
    }
    printf("case 4\n");
    node->rc = right_rotate(node->rc);
    node->rc->parent = node;

    return left_rotate(node);
  }
  return node;
}

void insert_tree(Tree* tree, Node* n) {
  assert(tree != NULL && n != NULL);
  if (tree->root == NULL) {
    tree->root = n;
    return;
  }
  insert_subtree(tree->root, n);
  tree->root = rebalance_node(tree->root);
}

Node* new_node(void* value, int key) {
  Node* node = malloc(sizeof(Node));
  assert(node != NULL);
  node->parent = node->lc = node->rc = NULL;
  node->value = value;
  node->key = key;
  node->height = 1;
  return node;
}

void print_node(Node* node) {
  if (node == NULL)
    return;

  printf("node: %d (h:%d) (b:%d) ", node->key, get_height(node), balance(node));
  if (node->parent)
    printf("p: %d ", node->key);
  if (node->lc != NULL)
    printf("lc: %d ", node->lc->key);
  if (node->rc != NULL)
    printf("rc: %d ", node->rc->key);
  printf("\n");
  print_node(node->lc);
  print_node(node->rc);
}

void print_tree(Tree* tree) {
  assert(tree != NULL);
  if (tree->root == NULL)
    return;
  while (tree->root->parent != NULL) {
    tree->root = tree->root->parent;
  }
  print_node(tree->root);
}
