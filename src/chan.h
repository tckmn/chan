#ifndef __CHAN_H__
#define __CHAN_H__

#include <ncurses.h>
#include <curl/curl.h>

struct chan {
    CURL *curl;
};

struct chan *chan_init();
void chan_main_loop(struct chan *chan);
void chan_destroy(struct chan *chan);

#endif
