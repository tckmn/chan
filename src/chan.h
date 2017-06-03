#ifndef __CHAN_H__
#define __CHAN_H__

#include <ncurses.h>
#include <curl/curl.h>

#define PAIR_RED     1
#define PAIR_GREEN   2
#define PAIR_YELLOW  3
#define PAIR_BLUE    4
#define PAIR_MAGENTA 5
#define PAIR_CYAN    6
#define PAIR_WHITE   7

#define PAIR_RED_BG     8
#define PAIR_GREEN_BG   9
#define PAIR_YELLOW_BG  10
#define PAIR_BLUE_BG    11
#define PAIR_MAGENTA_BG 12
#define PAIR_CYAN_BG    13
#define PAIR_WHITE_BG   14

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
    char *submission_fs;
    struct submission *viewing;
    char **view_buf;
    struct fmt **view_buf_fmt;
    int view_lines;
    int view_scroll;
    int active_comment;
    int *comment_offsets;
    char view_urlnbuf[VIEW_URLNBUF_LEN];
    char *username;
    char *password;
    int authenticated;
};

#define FMT_USER 0
#define FMT_AGE  1
#define FMT_URL  2
#define FMT_BAD  3
struct fmt {
    int type;
    int offset;
    int len;
    struct fmt *next;
};

struct submission {
    int id;
    char *auth;
    int voted;
    char *url;
    char *title;
    int job;
    int score;
    char *user;
    char *age;
    struct comment *comments;
    int ncomments;
    char **urls;
    int nurls;
};

struct comment {
    int id;
    int depth;
    char *auth;
    int voted;
    char *user;
    char *age;
    int badness;
    char *text;
};

struct chan *chan_init(int argc, char **argv);
void chan_main_loop(struct chan *chan);
void chan_destroy(struct chan *chan);

#endif
