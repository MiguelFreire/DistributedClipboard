
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

void logs(char *msg, int type)
{
    time_t rawtime;
    struct tm *t;

    time(&rawtime);
    t = localtime(&rawtime);

    switch(type) {
        case L_ERROR:
            printf("\x1b[31m[%02d:%02d:%02d][ERROR] - %s\x1b[0m \n", t->tm_hour, t->tm_min, t->tm_sec, msg);
            break;
        case L_INFO:
            printf("\x1b[34m[%02d:%02d:%02d][INFO] - %s\x1b[0m \n", t->tm_hour, t->tm_min, t->tm_sec, msg);
            break;
        default:
            printf("[%02d:%02d:%02d] - %s \n", t->tm_hour, t->tm_min, t->tm_sec, msg);
            break;
    }
    
}

/* Memory Safe Function */
void *smalloc(const size_t size)
{
    void *pointer = (void *)malloc(size);

    if (pointer == NULL)
        exit(0);

    return pointer;
}

void *scalloc(const size_t n, const size_t size)
{
    void *pointer = (void *)calloc(n, size);

    if (pointer == NULL)
        exit(0);

    return pointer;
}

void *srealloc(void *pointer, const size_t newSize)
{
    pointer = realloc(pointer, newSize);

    if (pointer == NULL)
    {
        exit(0);
    }

    return pointer;
}

/*File Helper Functions*/
void *sfopen(const char *fileName, const char *mode)
{
    FILE *file = fopen(fileName, mode);

    if (file == NULL)
    {
        exit(0);
    }

    return file;
}

int readAll(int sock_fd, void *buf, int count)
{
    int total = 0;
    int bytes_remaining = count;
    int bytes_received = 0;
    
    while (total < count)
    {   
        bytes_received = read(sock_fd, buf + total, bytes_remaining);
        total += bytes_received;
        bytes_remaining -= bytes_received;
    }

    return bytes_received;
}