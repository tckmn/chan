#ifndef __PARSE_H__
#define __PARSE_H__

#include <stdlib.h>
#include <string.h>

// jump to the position immediately after the nth > character
char *jumptag(char *str, int n) {
    do {
        str = strchr(str, '>') + 1;
    } while (--n);
    return str;
}

// allocate space for and copy src to dest until reaching delimiter
void copyuntil(char **dest, char *src, char delimiter) {
    int len = strchr(src, delimiter) - src;
    *dest = malloc(len + 1);
    strncpy(*dest, src, len);
    (*dest)[len] = '\0';
}

#endif
