#include "chan.h"

#include <stdlib.h>

struct chan *chan_init() {
    struct chan *chan = malloc(sizeof *chan);

    // ncurses initialization
    initscr();
    raw();
    noecho();

    // curl initialization
    curl_global_init(CURL_GLOBAL_DEFAULT);
    chan->curl = curl_easy_init();

    return chan;
}

void chan_main_loop(struct chan *chan) {
    curl_easy_setopt(chan->curl, CURLOPT_URL, "https://news.ycombinator.com/");
    curl_easy_setopt(chan->curl, CURLOPT_WRITEFUNCTION, printw);
    curl_easy_perform(chan->curl);
    refresh();
    getch();
}

void chan_destroy(struct chan *chan) {
    // ncurses cleanup
    endwin();

    // curl cleanup
    curl_global_cleanup();

    free(chan);
}
