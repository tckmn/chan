#ifndef __PARSE_H__
#define __PARSE_H__

// jump to the position immediately after the nth > character
char *jumpch(char *str, char ch, int n);
#define jumptag(str,n) jumpch(str,'>',n)
#define jumpapos(str,n) jumpch(str,'\'',n)
#define jumpquot(str,n) jumpch(str,'"',n)

// allocate space for and copy src to dest until reaching delimiter with offset
void copyuntiloff(char **dest, char *src, char delimiter, int offset);

// allocate space for and copy src to dest until reaching delimiter
void copyuntil(char **dest, char *src, char delimiter);

// allocate space for and copy src to dest, where src starts with a relative timestamp
void copyage(char **dest, char *src);

#endif
