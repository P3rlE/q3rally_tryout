#ifndef LCC_STDLIB_H
#define LCC_STDLIB_H

#include "stddef.h"

void *malloc(size_t size);
void free(void *ptr);
void exit(int status);
int atoi(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);

#endif /* LCC_STDLIB_H */
