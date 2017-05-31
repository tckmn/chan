#include "chan.h"

#include <stdlib.h>
#include <string.h>

struct membuf {
    char *data;
    size_t len;
};

size_t mem_callback(char *data, size_t size, size_t nmemb, void *userp) {
    size_t len = size * nmemb;
    struct membuf *buf = (struct membuf *) userp;
    buf->data = realloc(buf->data, buf->len + len);
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    return len;
}

char *do_GET(struct chan *chan, const char *url) {
    struct membuf buf;
    buf.data = NULL;
    buf.len = 0;

    curl_easy_setopt(chan->curl, CURLOPT_URL, url);
    curl_easy_setopt(chan->curl, CURLOPT_WRITEFUNCTION, mem_callback);
    curl_easy_setopt(chan->curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_perform(chan->curl);

    return buf.data;
}

// jump to the position immediately after the nth > character
char *jumptag(char *str, int n) {
    do {
        str = strchr(str, '>') + 1;
    } while (--n);
    return str;
}

// allocate space for and copy src to dest until reaching delimiter
void copyuntil(char **dest, char *src, char delimiter) {
    int len = strchr(src, delimiter) - src;
    *dest = malloc(len + 1);
    strncpy(*dest, src, len);
    (*dest)[len] = '\0';
}

void chan_destroy_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        free(chan->submissions[i].url);
        free(chan->submissions[i].title);
        free(chan->submissions[i].user);
    }
    free(chan->submissions);
    chan->submissions = NULL;
    chan->nsubmissions = 0;
}

void chan_update_submissions(struct chan *chan) {
    chan_destroy_submissions(chan);
    chan->submissions = malloc(30 * sizeof *chan->submissions);
    chan->nsubmissions = 30;

    char *data = do_GET(chan, "https://news.ycombinator.com/");

    struct submission *submission = chan->submissions;
    while ((data = strstr(data, "<tr class='athing'"))) {
        data += strlen("<tr class='athing' id='");
        submission->id = atoi(data);

        data = strstr(data, "<td class=\"title\"><a href=\"") +
            strlen("<td class=\"title\"><a href=\"");
        copyuntil(&submission->url, data, '"');

        data = jumptag(data, 1);
        copyuntil(&submission->title, data, '<');

        // next line always starts with <span class="
        data = strchr(strchr(data, '\n'), '=') + 2;
        if (*data == 's') {
            // "score" i.e. it's a regular submission
            submission->job = 0;

            data = jumptag(data, 1);
            submission->score = atoi(data);

            data = jumptag(data, 2);
            copyuntil(&submission->user, data, '<');

            data = jumptag(data, 3);
        } else if (*data == 'a') {
            // "age" i.e. it's one of those job posts
            submission->job = 1;

            submission->score = 0;

            submission->user = malloc(1);
            submission->user[0] = '\0';

            data = jumptag(data, 2);
        } else exit(123); // TODO something better
        submission->age = atoi(data);
        if (data[2 + (submission->age >= 10 ? 1 : 0)] == 'd') {
            submission->age *= 24;
        }

        if (submission->job) {
            submission->comments = 0;
        } else {
            data = jumptag(data, 7);
            submission->comments = atoi(data);
        }

        ++submission;
    }
}

void chan_draw_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        struct submission submission = chan->submissions[i];
        if (submission.job) {
            printw("    %2dh     %s\n", submission.age, submission.title);
        } else {
            printw("%3d %2dh %3d %s\n", submission.score, submission.age, submission.comments, submission.title);
        }
    }
    refresh();
}

struct chan *chan_init() {
    struct chan *chan = malloc(sizeof *chan);
    chan->submissions = NULL;
    chan->nsubmissions = 0;

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
    chan_update_submissions(chan);
    chan_draw_submissions(chan);
    getch();
}

void chan_destroy(struct chan *chan) {
    // ncurses cleanup
    endwin();

    // curl cleanup
    curl_global_cleanup();

    chan_destroy_submissions(chan);
    free(chan);
}
