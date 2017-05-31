#include "chan.h"
#include "submissions.h"
#include "comments.h"

#include <stdlib.h>

struct chan *chan_init() {
    struct chan *chan = malloc(sizeof *chan);
    chan->submissions = NULL;
    chan->nsubmissions = 0;
    chan->active_submission = 0;
    chan->viewing = NULL;

    // ncurses initialization
    initscr();
    raw();
    noecho();
    curs_set(0);

    chan->main_win = newwin(LINES - 1, COLS, 0, 0);
    chan->status_win = newwin(1, COLS, LINES - 1, 0);
    refresh();

    // curl initialization
    curl_global_init(CURL_GLOBAL_DEFAULT);
    chan->curl = curl_easy_init();

    return chan;
}

void chan_main_loop(struct chan *chan) {
    chan_update_submissions(chan);
    chan_draw_submissions(chan);

    int ch;
    while ((ch = getch())) {
        if (ch == 'q') break;
        if (chan->viewing) chan_comments_key(chan, ch);
        else chan_submissions_key(chan, ch);
    }
}

void chan_destroy(struct chan *chan) {
    // ncurses cleanup
    endwin();

    // curl cleanup
    curl_global_cleanup();

    chan_destroy_submissions(chan);
    free(chan);
}
