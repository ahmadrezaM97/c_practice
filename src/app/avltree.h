#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Node {
  void* value;
  int key;
  struct Node* lc;
  struct Node* rc;
  struct Node* parent;
  int height;
} Node;

typedef struct {
  Node* root;
} Tree;

void* find_in_tree(Tree* tree, int key);
void insert_tree(Tree* tree, Node* n);
Node* new_node(void* value, int key);
void print_tree(Tree* tree);