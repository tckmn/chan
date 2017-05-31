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

// strstr, but returns the character immediately after the match
// only use this if you know there will be a match!
#define strstra(haystack,needle) (strstr((haystack),(needle))+strlen(needle))

// jump to the position immediately after the nth > character
char *jumptag(char *str, int n) {
    do {
        str = strchr(str, '>') + 1;
    } while (--n);
    return str;
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

        data = strstra(data, "<td class=\"title\"><a href=\"");
        int url_len = strchr(data, '"') - data;
        submission->url = malloc(url_len + 1);
        strncpy(submission->url, data, url_len);
        submission->url[url_len] = '\0';

        data = jumptag(data, 1);
        int title_len = strchr(data, '<') - data;
        submission->title = malloc(title_len + 1);
        strncpy(submission->title, data, title_len);
        submission->title[title_len] = '\0';

        // next line always starts with <span class="
        data = strchr(strchr(data, '\n'), '=') + 2;
        if (*data == 's') {
            // "score" i.e. it's a regular submission
            data = jumptag(data, 1);
            submission->score = atoi(data);

            data = jumptag(data, 2);
            int user_len = strchr(data, '<') - data;
            submission->user = malloc(user_len + 1);
            strncpy(submission->user, data, user_len);
            submission->user[user_len] = '\0';

            data = jumptag(data, 3);
            submission->age = atoi(data);
            if (data[2 + (submission->age >= 10 ? 1 : 0)] == 'd') {
                submission->age *= 24;
            }

            data = jumptag(data, 7);
            submission->comments = atoi(data);
        } else if (*data == 'a') {
            // "age" i.e. it's one of those job posts
            submission->score = 0;
            submission->user = malloc(1);
            submission->user[0] = '\0';
            submission->comments = 0;

            data = jumptag(data, 2);
            submission->age = atoi(data);
            if (data[2 + (submission->age >= 10 ? 1 : 0)] == 'd') {
                submission->age *= 24;
            }
        } else {
            // TODO ??? what happened here
        }

        ++submission;
    }
}

void chan_draw_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        struct submission submission = chan->submissions[i];
        if (submission.score) {
            printw("%3d %2dh %3d %s\n", submission.score, submission.age, submission.comments, submission.title);
        } else {
            printw("    %2dh     %s\n", submission.age, submission.title);
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
