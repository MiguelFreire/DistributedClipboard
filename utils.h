#ifndef UTILS_H
#define UTILS_H


#include <stdio.h>
#include <stdlib.h>

#define L_ERROR -1
#define L_INFO  0
/* Console helpers */

void logs(char* msg, int type);

/* Memory Safe Function */
void *smalloc(const size_t size);

void *scalloc(const size_t n, const size_t size);

void *srealloc(void *pointer, const size_t newSize);

/*File Helper Functions*/
void *sfopen(const char *fileName, const char *mode);

#endif