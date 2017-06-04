#include "config.h"
#include "sys.h"
#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 50

#define NEEDS_VAL do {                                                        \
    if (!val) {                                                               \
        fprintf(stderr, "option `%.*s' requires a value\n", key_len, key);    \
        return 1;                                                             \
    }                                                                         \
} while (0)


#define PARSE_KEY(ident) do {                                                 \
    if (!strncmp(key, #ident, key_len)) {                                     \
        if (!val || strlen(val) != 1) {                                       \
            fprintf(stderr, "option `%.*s' requires exactly one character\n", \
                    key_len, key);                                            \
            return 1;                                                         \
        }                                                                     \
        if (overwrite || !chan->keys.ident) {                                 \
            chan->keys.ident = *val;                                          \
        }                                                                     \
        return 0;                                                             \
    }                                                                         \
} while (0)

int parse_long_arg(struct chan *chan, char *key, int key_len, char *val, int overwrite) {
    if (!strncmp(key, "username", key_len)) {
        NEEDS_VAL;
        if (overwrite || !chan->username) {
            free(chan->username);
            chan->username = malloc(strlen(val) + 1);
            strcpy(chan->username, val);
        }
        return 0;
    }

    if (!strncmp(key, "password", key_len)) {
        NEEDS_VAL;
        if (overwrite || !chan->password) {
            free(chan->password);
            chan->password = malloc(strlen(val) + 1);
            strcpy(chan->password, val);
        }
        return 0;
    }

    PARSE_KEY(sub_down);
    PARSE_KEY(sub_up);
    PARSE_KEY(sub_login);
    PARSE_KEY(sub_open_url);
    PARSE_KEY(sub_reload);
    PARSE_KEY(sub_upvote);
    PARSE_KEY(sub_view_comments);
    PARSE_KEY(sub_home);
    PARSE_KEY(sub_new);
    PARSE_KEY(sub_show);
    PARSE_KEY(sub_ask);
    PARSE_KEY(sub_jobs);
    PARSE_KEY(com_scroll_down);
    PARSE_KEY(com_scroll_up);
    PARSE_KEY(com_next);
    PARSE_KEY(com_prev);
    PARSE_KEY(com_next_at_depth);
    PARSE_KEY(com_prev_at_depth);
    PARSE_KEY(com_open_url);
    PARSE_KEY(com_back);
    PARSE_KEY(com_upvote);

    fprintf(stderr, "unknown option `%.*s'\n", key_len, key);
    return 1;
}

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

        if (parse_long_arg(chan, line, eqpos - line, eqpos + 1, overwrite)) {
            fprintf(stderr, "(from config file %s:%d)\n", path, lineno);
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
    chan->sub.fmt_str = "%s %a %c %t";

    // parse arguments
    for (int i = 1; i < argc; ++i) {
        if (parse_options && argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                if (argv[i][2]) {
                    // --thing, parse as long argument
                    char *longarg = argv[i] + 2, *eqpos;
                    if (!strcmp(longarg, "help")) {
                        help = 1;
                        continue;
                    }
                    if (!strcmp(longarg, "version")) {
                        version = 1;
                        continue;
                    }

                    int res;
                    if ((eqpos = strchr(longarg, '='))) {
                        res = parse_long_arg(chan, longarg, eqpos - longarg,
                                eqpos + 1, 1);
                    } else {
                        res = parse_long_arg(chan, longarg, strlen(longarg),
                                argv[++i], 1);
                    }

                    if (res) {
                        fputs("(from long command line option)\n", stderr);
                        return 1;
                    }
                } else {
                    // --, stop parsing options
                    parse_options = 0;
                }
            } else {
                // -xyz, parse as short option(s)
                int res = 0, consumed = 0;

                for (char *c = argv[i] + 1; *c; ++c) {
                    char *val = c + 1;
                    if (!*val) {
                        consumed = 1;
                        val = i < argc - 1 ? argv[i+1] : NULL;
                    }

                    switch (*c) {
                        case 'h':
                            help = 1;
                            break;
                        case 'v':
                            version = 1;
                            break;
                        case 'u':
                            res = parse_long_arg(chan, "username", 8, val, 1);
                            goto parsed_long;
                        case 'p':
                            res = parse_long_arg(chan, "password", 8, val, 1);
                            goto parsed_long;
                        default:
                            fprintf(stderr, "unknown short option -%c\n", *c);
                            return 1;
                    }
                }
                continue;

parsed_long:
                if (res) {
                    fputs("(from short command line option)\n", stderr);
                    return 1;
                }
                if (consumed) ++i;
            }
        } else {
            // bare argument, parse as config file path
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
        chan->authenticated = 1;
    }
    free(chan->username);
    free(chan->password);
    chan->username = chan->password = NULL;

    // defaults...
    if (!chan->keys.sub_down)          chan->keys.sub_down = 'j';
    if (!chan->keys.sub_up)            chan->keys.sub_up = 'k';
    if (!chan->keys.sub_login)         chan->keys.sub_login = 'l';
    if (!chan->keys.sub_open_url)      chan->keys.sub_open_url = 'o';
    if (!chan->keys.sub_reload)        chan->keys.sub_reload = 'r';
    if (!chan->keys.sub_upvote)        chan->keys.sub_upvote = '+';
    if (!chan->keys.sub_view_comments) chan->keys.sub_view_comments = '\n';
    if (!chan->keys.sub_home)          chan->keys.sub_home = 'H';
    if (!chan->keys.sub_new)           chan->keys.sub_new = 'N';
    if (!chan->keys.sub_show)          chan->keys.sub_show = 'S';
    if (!chan->keys.sub_ask)           chan->keys.sub_ask = 'A';
    if (!chan->keys.sub_jobs)          chan->keys.sub_jobs = 'J';
    if (!chan->keys.com_scroll_down)   chan->keys.com_scroll_down = 'e';
    if (!chan->keys.com_scroll_up)     chan->keys.com_scroll_up = 'y';
    if (!chan->keys.com_next)          chan->keys.com_next = 'j';
    if (!chan->keys.com_prev)          chan->keys.com_prev = 'k';
    if (!chan->keys.com_next_at_depth) chan->keys.com_next_at_depth = 'J';
    if (!chan->keys.com_prev_at_depth) chan->keys.com_prev_at_depth = 'K';
    if (!chan->keys.com_open_url)      chan->keys.com_open_url = 'o';
    if (!chan->keys.com_back)          chan->keys.com_back = 'q';
    if (!chan->keys.com_upvote)        chan->keys.com_upvote = '+';

    return 0;
}
