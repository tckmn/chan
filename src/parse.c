#include "parse.h"

#include <stdlib.h>
#include <string.h>

char *jumpch(char *str, char ch, int n) {
    do {
        str = strchr(str, ch) + 1;
    } while (--n);
    return str;
}
#define jumptag(str,n) jumpch(str,'>',n)
#define jumpapos(str,n) jumpch(str,'\'',n)
#define jumpquot(str,n) jumpch(str,'"',n)

void copyuntil(char **dest, char *src, char delimiter) {
    int len = strchr(src, delimiter) - src;
    *dest = malloc(len + 1);
    strncpy(*dest, src, len);
    (*dest)[len] = '\0';
}

void copyage(char **dest, char *src) {
    int len = strchr(src, ' ') - src + 1;
    *dest = malloc(len + 1);
    strncpy(*dest, src, len + 1);
    (*dest)[len - 1] = (*dest)[len];
    (*dest)[len] = '\0';
}
