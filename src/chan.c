#include "chan.h"
#include "submissions.h"
#include "comments.h"
#include "login.h"
#include "config.h"

#include <stdlib.h>

struct chan *chan_init(int argc, char **argv) {
    // chan initialization
    struct chan *chan = calloc(1, sizeof *chan);

    // curl initialization
    // (we only do this in the middle of initializing chan because in case a
    //  username or password is passed as a config option, we need it to
    //  authenticate)
    curl_global_init(CURL_GLOBAL_DEFAULT);
    chan->curl = curl_easy_init();
    // start cookie engine
    curl_easy_setopt(chan->curl, CURLOPT_COOKIEFILE, "");

    // chan initialization (episode 2)
    if (chan_config(chan, argc, argv)) {
        free(chan);
        return NULL;
    }

    // ncurses initialization
    initscr();
    raw();
    noecho();
    curs_set(0);
    start_color();
    init_pair(PAIR_RED,     COLOR_RED,     COLOR_BLACK);
    init_pair(PAIR_GREEN,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(PAIR_YELLOW,  COLOR_YELLOW,  COLOR_BLACK);
    init_pair(PAIR_BLUE,    COLOR_BLUE,    COLOR_BLACK);
    init_pair(PAIR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(PAIR_CYAN,    COLOR_CYAN,    COLOR_BLACK);
    init_pair(PAIR_WHITE,   COLOR_WHITE,   COLOR_BLACK);

    chan->main_win = newwin(chan->main_lines = LINES - 1,
            chan->main_cols = COLS, 0, 0);
    scrollok(chan->main_win, TRUE);
    chan->status_win = newwin(1, COLS, LINES - 1, 0);
    refresh();

    return chan;
}

void chan_main_loop(struct chan *chan) {
    chan_update_submissions(chan);
    chan_draw_submissions(chan);

    int ch;
    while ((ch = getch())) {
        int handled;
        if (chan->username)     handled = chan_login_key(chan, ch);
        else if (chan->viewing) handled = chan_comments_key(chan, ch);
        else                    handled = chan_submissions_key(chan, ch);

        // global keybinds
        if (!handled) switch (ch) {
            case 'q': goto finish;
        }
    }

    // label to break out of the main loop
    finish:;
}

void chan_destroy(struct chan *chan) {
    // ncurses cleanup
    endwin();

    // curl cleanup
    curl_easy_cleanup(chan->curl);
    curl_global_cleanup();

    chan_destroy_submissions(chan);
    free(chan);
}
