#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // write
#include <zlib.h>
#include <stdbool.h>


struct node {
    int value;
    struct node* left;
    struct node* right;
};

typedef struct {
    int value;
    tnod* left;
    tnod* right;
} tnod;

void (*signal(int, void (*)(int)))(int);

int main() {
    struct node n = { .value = 10 };
    return 0;
}