#ifndef __CHAN_H__
#define __CHAN_H__

#include <ncurses.h>
#include <curl/curl.h>

struct chan {
    CURL *curl;
    WINDOW *main_win;
    WINDOW *status_win;
    struct submission *submissions;
    int nsubmissions;
    int active_submission;
};

struct submission {
    int id;
    char *url;
    char *title;
    int job;
    int score;
    char *user;
    char *age;
    int comments;
};

struct chan *chan_init();
void chan_main_loop(struct chan *chan);
void chan_destroy(struct chan *chan);

#endif
