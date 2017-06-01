#ifndef __CHAN_H__
#define __CHAN_H__

#include <ncurses.h>
#include <curl/curl.h>

#define VIEW_URLNBUF_LEN 4
struct chan {
    CURL *curl;
    WINDOW *main_win;
    int main_lines;
    int main_cols;
    WINDOW *status_win;
    struct submission *submissions;
    int nsubmissions;
    int active_submission;
    struct submission *viewing;
    char **view_buf;
    struct fmt **view_buf_fmt;
    int view_lines;
    int view_scroll;
    char **view_urls;
    int view_nurls;
    char view_urlnbuf[VIEW_URLNBUF_LEN];
};

#define FMT_USER 0
#define FMT_AGE  1
#define FMT_URL  2
struct fmt {
    int type;
    int offset;
    int len;
    struct fmt *next;
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
