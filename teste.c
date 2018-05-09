#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct A {
    char *c;
    int size;
} A;


int main() {
    char buffer[] = "Teste";
    A *p = (A *) malloc(sizeof(A) + strlen(buffer)+1);
    int *k;
    p->size = strlen(buffer)+1;
    p->c = (char *) malloc(p->size);

    strcpy(p->c, buffer);
    printf("Struct size: %d\n", sizeof(struct A));
    printf("Size: %d\n", sizeof(k));

    free(p->c);
    free(p);
    return 0;
}