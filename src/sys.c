#include "sys.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// TODO fallback if no xdg-utils
// TODO cross platform
void urlopen(const char *url) {
    char *cmd = malloc(strlen(url) + 12);
    sprintf(cmd, "xdg-open '%s'", url);
    system(cmd);
    free(cmd);
}

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#endif
char *default_config_file() {
    char *homedir, *buf = NULL;

#ifdef _WIN32
    if ((homedir = getenv("USERPROFILE")) == NULL &&
            (homedir = getenv("APPDATA")) == NULL) {
        // eh, it's windows, who really cares
        homedir = "C:";
    }
#else
    // first try $HOME
    if ((homedir = getenv("HOME")) == NULL) {
        // if that fails, go through the passwd file
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            homedir = pw->pw_dir;
        } else {
            // nothing we can do works, fall back to current working directory
            size_t bufsize = 100;
            buf = malloc(bufsize);
            while (!getcwd(buf, bufsize)) {
                if (errno != ERANGE) {
                    // literally nothing is working! guess we'll just have to
                    // check the root directory...
                    homedir = "";
                    break;
                }
                bufsize += 100;
                buf = realloc(buf, bufsize);
                homedir = buf;
            }
        }
    }
#endif

    char *conf = malloc(strlen(homedir) + 9);
    strcpy(conf, homedir);
    strcat(conf,
#ifdef _WIN32
                 "\\_chanrc"
#else
                 "/.chanrc"
#endif
                            );

    free(buf);
    return conf;
}
