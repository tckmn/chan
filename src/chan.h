#ifndef __CHAN_H__
#define __CHAN_H__

#include <ncurses.h>
#include <curl/curl.h>

struct chan {
    CURL *curl;
    struct submission *submissions;
    int nsubmissions;
};

struct submission {
    int id;
    char *url;
    char *title;
    int score;
    char *user;
    int age;
    int comments;
};

struct chan *chan_init();
void chan_main_loop(struct chan *chan);
void chan_destroy(struct chan *chan);

#endif
