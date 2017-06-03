#include "parse.h"

#include <stdlib.h>
#include <string.h>

/*
 * return a pointer to the position immediately after the nth occurrence of ch
 * in str
 */
char *jumpch(char *str, char ch, int n) {
    do {
        str = strchr(str, ch) + 1;
    } while (--n);
    return str;
}
#define jumptag(str,n) jumpch(str,'>',n)
#define jumpapos(str,n) jumpch(str,'\'',n)
#define jumpquot(str,n) jumpch(str,'"',n)

/*
 * allocate space for and copy into *dest everything from the beginning of src
 * to the first occurrence of delimiter
 */
void copyuntil(char **dest, char *src, char delimiter) {
    int len = strchr(src, delimiter) - src;
    *dest = malloc(len + 1);
    strncpy(*dest, src, len);
    (*dest)[len] = '\0';
}

/*
 * special case of copyuntil for submission/comment ages, where src starts with
 * a relative timestamp
 */
void copyage(char **dest, char *src) {
    int len = strchr(src, ' ') - src + 1;
    *dest = malloc(len + 1);
    strncpy(*dest, src, len + 1);
    (*dest)[len - 1] = (*dest)[len];
    (*dest)[len] = '\0';
}
