#ifndef __PARSE_H__
#define __PARSE_H__

char *jumpch(char *str, char ch, int n);
#define jumptag(str,n) jumpch(str,'>',n)
#define jumpapos(str,n) jumpch(str,'\'',n)
#define jumpquot(str,n) jumpch(str,'"',n)

void copyuntil(char **dest, char *src, char delimiter);

void copyage(char **dest, char *src);

#endif
