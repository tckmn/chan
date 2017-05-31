#include "chan.h"
#include "net.h"

#include <stdlib.h>
#include <string.h>

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

    char *data = do_GET(chan->curl, "https://news.ycombinator.com/");

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
        int age_len = strchr(data, ' ') - data + 1;
        submission->age = malloc(age_len + 1);
        strncpy(submission->age, data, age_len + 1);
        submission->age[age_len - 1] = submission->age[age_len];
        submission->age[age_len] = '\0';

        if (submission->job) {
            submission->comments = 0;
        } else {
            data = jumptag(data, 7);
            submission->comments = atoi(data);
        }

        ++submission;
    }
}

// does NOT call wrefresh()!
void chan_redraw_submission(struct chan *chan, int i) {
    wattrset(chan->main_win, i == chan->active_submission ? A_REVERSE : 0);
    struct submission submission = chan->submissions[i];
    char *line = malloc(COLS + 1);
    int written;
    if (submission.job) {
        written = snprintf(line, COLS + 1, "    %3s     %s", submission.age, submission.title);
    } else {
        written = snprintf(line, COLS + 1, "%3d %3s %3d %s", submission.score, submission.age, submission.comments, submission.title);
    }
    if (written < COLS) {
        memset(line + written, ' ', COLS - written);
        line[COLS] = '\0';
    }
    mvwaddstr(chan->main_win, i, 0, line);
}

void chan_draw_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        chan_redraw_submission(chan, i);
    }
    wrefresh(chan->main_win);
}

void chan_submissions_key(struct chan *chan, int ch) {
    switch (ch) {
        case 'j':
            if (chan->active_submission < chan->nsubmissions - 1) {
                ++chan->active_submission;
            }
            chan_redraw_submission(chan, chan->active_submission - 1);
            chan_redraw_submission(chan, chan->active_submission);
            wrefresh(chan->main_win);
            break;
        case 'k':
            if (chan->active_submission > 0) {
                --chan->active_submission;
            }
            chan_redraw_submission(chan, chan->active_submission + 1);
            chan_redraw_submission(chan, chan->active_submission);
            wrefresh(chan->main_win);
            break;
        case 'o': {
            // TODO do this in a better way
            char *url = chan->submissions[chan->active_submission].url;
            char *cmd = malloc(strlen(url) + 12);
            sprintf(cmd, "xdg-open '%s'", url);
            system(cmd);
            break;
        }
    }
}

struct chan *chan_init() {
    struct chan *chan = malloc(sizeof *chan);
    chan->submissions = NULL;
    chan->nsubmissions = 0;
    chan->active_submission = 0;

    // ncurses initialization
    initscr();
    raw();
    noecho();
    curs_set(0);

    chan->main_win = newwin(LINES - 1, COLS, 0, 0);
    chan->status_win = newwin(1, COLS, LINES - 1, 0);
    refresh();

    // curl initialization
    curl_global_init(CURL_GLOBAL_DEFAULT);
    chan->curl = curl_easy_init();

    return chan;
}

void chan_main_loop(struct chan *chan) {
    chan_update_submissions(chan);
    chan_draw_submissions(chan);

    int ch;
    while ((ch = getch())) {
        if (ch == 'q') break;
        chan_submissions_key(chan, ch);
    }
}

void chan_destroy(struct chan *chan) {
    // ncurses cleanup
    endwin();

    // curl cleanup
    curl_global_cleanup();

    chan_destroy_submissions(chan);
    free(chan);
}
