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
    struct submission *viewing;
    char **view_buf;
    int view_lines;
};

struct submission {
    int id;
    char *url;
    char *title;
    int job;
    int score;
    char *user;
    char *age;
    struct comment *comments;
    int ncomments;
};

struct comment {
    int id;
    int depth;
    char *user;
    char *age;
    int badness;
    char *text;
};

struct chan *chan_init();
void chan_main_loop(struct chan *chan);
void chan_destroy(struct chan *chan);

#endif
