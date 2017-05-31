#include "submissions.h"
#include "comments.h"

#include <stdlib.h>
#include <string.h>

void chan_destroy_comments(struct chan *chan) {
}

void chan_update_comments(struct chan *chan) {
}

void chan_draw_comments(struct chan *chan) {
    wclear(chan->main_win);
    wattrset(chan->main_win, 0);
    waddstr(chan->main_win, chan->viewing->title);
    wrefresh(chan->main_win);
}

void chan_comments_key(struct chan *chan, int ch) {
    switch (ch) {
        case 'o': {
            // TODO do this in a better way
            // TODO don't duplicate code from submissions.c
            char *url = chan->viewing->url;
            char *cmd = malloc(strlen(url) + 12);
            sprintf(cmd, "xdg-open '%s'", url);
            system(cmd);
            break;
        }
        case 'q':
            chan->viewing = NULL;
            chan_draw_submissions(chan);
            break;
    }
}
