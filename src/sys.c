#include "sys.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void urlopen(const char *url) {
    char *cmd = malloc(strlen(url) + 12);
    sprintf(cmd, "xdg-open '%s'", url);
    system(cmd);
    free(cmd);
}
