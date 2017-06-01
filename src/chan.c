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
    chan->view_buf = NULL;
    chan->view_buf_fmt = NULL;
    chan->view_lines = 0;
    chan->view_scroll = 0;
    chan->view_urlnbuf[0] = '\0';

    // ncurses initialization
    initscr();
    raw();
    noecho();
    curs_set(0);
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);

    chan->main_win = newwin(chan->main_lines = LINES - 1,
            chan->main_cols = COLS, 0, 0);
    scrollok(chan->main_win, TRUE);
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
        if (chan->viewing) chan_comments_key(chan, ch);
        else {
            if (ch == 'q') break;
            chan_submissions_key(chan, ch);
        }
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
