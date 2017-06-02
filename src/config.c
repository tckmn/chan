#include "config.h"
#include "sys.h"
#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define strnncmp(s1,s2,n) ((n) ? strncmp((s1),(s2),(n)) : strcmp((s1),(s2)))
#define BUF_SIZE 50

int config_from_file(struct chan *chan, char *path, int overwrite) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (overwrite) {
            fprintf(stderr, "failed to open file %s for reading\n", path);
            return 1;
        } else return 0;
    }

    char *line = NULL;
    for (int lineno = 1;; ++lineno) {
        size_t len = 1, end = 0;
        do {
            line = realloc(line, len += BUF_SIZE);
            // since we're overwriting the existing NUL terminator, add 1
            if (!fgets(line + end, BUF_SIZE + 1, fp)) {
                free(line);
                return 0;
            }
            end += strlen(line + end);
        } while (line[end - 1] != '\n');

        // remove the trailing newline
        line[end - 1] = '\0';

        // parse
        char *eqpos = strchr(line, '=');
        if (!eqpos) {
            fprintf(stderr, "malformed config option at %s:%d\n", path, lineno);
            free(line);
            return 1;
        }

        int optlen = eqpos - line;
        if (!strncmp(line, "username", optlen)) {
            if (!chan->username || overwrite) {
                free(chan->username);
                chan->username = malloc(strlen(eqpos));
                strcpy(chan->username, eqpos + 1);
            }
        } else if (!strncmp(line, "password", optlen)) {
            if (!chan->password || overwrite) {
                free(chan->password);
                chan->password = malloc(strlen(eqpos));
                strcpy(chan->password, eqpos + 1);
            }
        } else {
            fprintf(stderr, "unknown config option %.*s (at %s:%d)\n",
                    optlen, line, path, lineno);
            free(line);
            return 1;
        }
        free(line);
        line = NULL;
    }
}

int chan_config(struct chan *chan, int argc, char **argv) {
    int used_file = 0, parse_options = 1;
    int help = 0, version = 0;
    chan->submission_fs = "%s %a %c %t";

    // parse arguments
    for (int i = 1; i < argc; ++i) {
        if (parse_options && argv[i][0] == '-') {
            char **stroptdest = NULL, *stroptsrc = NULL;

            if (argv[i][1] == '-') {
                if (argv[i][2]) {
                    char *longarg = argv[i] + 2, *eqpos;
                    if (!strcmp(longarg, "help")) {
                        help = 1;
                        continue;
                    }
                    if (!strcmp(longarg, "version")) {
                        version = 1;
                        continue;
                    }

                    int n = 0;
                    if ((eqpos = strchr(longarg, '='))) {
                        n = eqpos - longarg;
                    }
                    if (!strnncmp(longarg, "username", n)) {
                        stroptdest = &chan->username;
                        stroptsrc = n ? longarg + n + 1 : "";
                    } else if (!strnncmp(longarg, "password", n)) {
                        stroptdest = &chan->password;
                        stroptsrc = n ? longarg + n + 1 : "";
                    } else {
                        fprintf(stderr, "unknown long option --%s\n", longarg);
                        return 1;
                    }
                } else {
                    // --, stop parsing options
                    parse_options = 0;
                }
            } else {
                for (char *c = argv[i] + 1; *c; ++c) {
                    switch (*c) {
                        case 'h':
                            help = 1;
                            break;
                        case 'v':
                            version = 1;
                            break;
                        case 'u':
                            stroptdest = &chan->username;
                            stroptsrc = c + 1;
                            goto breakloop;
                        case 'p':
                            stroptdest = &chan->password;
                            stroptsrc = c + 1;
                            goto breakloop;
                        default:
                            fprintf(stderr, "unknown short option -%c\n", *c);
                            return 1;
                    }
                }
                breakloop:;
            }

            if (stroptdest) {
                free(*stroptdest);
                char *ptr = *stroptsrc ? stroptsrc : argv[++i];
                *stroptdest = malloc(strlen(ptr) + 1);
                strcpy(*stroptdest, ptr);
            }
        } else {
            used_file = 1;
            int res = config_from_file(chan, argv[i], 1);
            if (res) return res;
        }
    }

    // if -h was present, output help and exit
    if (help) {
        fprintf(stderr, "usage: %s [OPTION...] [CONFIG-FILE...]\n"
                        "       %s [-h|-v]\n"
                        "\n"
                        "  -u, --username\n"
                        "  -p, --password   credentials to log in with\n"
                        "  -h, --help       display this help message\n"
                        "  -v, --version    display the installed version of chan\n"
                        , argv[0], argv[0]);
        return 1;
    }

    // if -v was present, output version and exit
    if (version) {
        fputs("chan 0.0.1\n", stderr);
        return 1;
    }

    // if no config files were specified, use the default
    if (!used_file) {
        char *path = default_config_file();
        int res = config_from_file(chan, path, 0);
        free(path);
        if (res) return res;
    }

    // if credientials were specified, use them to log in
    if (chan->username && chan->password) {
        if (!auth(chan->curl, chan->username, chan->password)) {
            fputs("the given credentials were invalid\n", stderr);
            return 1;
        }
    }
    free(chan->username);
    free(chan->password);
    chan->username = chan->password = NULL;

    return 0;
}
