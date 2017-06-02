#include "submissions.h"
#include "comments.h"
#include "net.h"
#include "parse.h"
#include "sys.h"
#include "utf8.h"

#include <stdlib.h>
#include <string.h>

void chan_destroy_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        free(chan->submissions[i].url);
        free(chan->submissions[i].title);
        free(chan->submissions[i].user);
        free(chan->submissions[i].comments);
        for (int j = 0; j < chan->submissions[i].nurls; ++j) {
            free(chan->submissions[i].urls[j]);
        }
        free(chan->submissions[i].urls);
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

        char *auth = strstr(data, "auth=");
        submission->voted = 0;
        if (auth && auth < jumpch(data, '\n', 2)) {
            copyuntil(&submission->auth, auth + strlen("auth="), '&');
            if (!strncmp(jumpapos(auth, 2), "nosee", 5)) submission->voted = 1;
        } else submission->auth = NULL;

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
        copyage(&submission->age, data);

        submission->comments = NULL;
        if (submission->job) {
            submission->ncomments = 0;
        } else {
            data = jumptag(data, 7);
            submission->ncomments = atoi(data);
        }

        ++submission;
    }
}

// does NOT call wrefresh()!
void chan_redraw_submission(struct chan *chan, int i) {
    wattrset(chan->main_win, i == chan->active_submission ? A_REVERSE : 0);
    struct submission submission = chan->submissions[i];

    int y, x;
    char *substr = NULL;
    int idx = 0, subidx = 0;
    wmove(chan->main_win, i, 0);
    do {
        if (substr) {
            int blen = bytelen(substr[subidx]);
            waddnstr(chan->main_win, substr + subidx, blen);
            if (!substr[subidx += blen]) {
                free(substr);
                substr = NULL;
                subidx = 0;
                wattroff(chan->main_win, COLOR_PAIR(2));
            }
        } else {
            if (!chan->submission_fs[idx]) {
                waddch(chan->main_win, ' ');
            } else if (chan->submission_fs[idx] == '%') {
                switch (chan->submission_fs[++idx]) {
                    case 's':
                        substr = malloc(5);
                        snprintf(substr, 5, "%4d", submission.score);
                        if (submission.voted) {
                            wattron(chan->main_win, COLOR_PAIR(2));
                        }
                        break;
                    case 'a':
                        substr = malloc(4);
                        snprintf(substr, 4, "%3s", submission.age);
                        break;
                    case 'c':
                        substr = malloc(4);
                        snprintf(substr, 4, "%3d", submission.ncomments);
                        break;
                    case 't':
                        substr = malloc(strlen(submission.title) + 1);
                        strcpy(substr, submission.title);
                        break;
                    case '%':
                        waddch(chan->main_win, '%');
                        break;
                    default:
                        waddch(chan->main_win, '%');
                        --idx;
                        break;
                }
                ++idx;
            } else {
                int blen = bytelen(chan->submission_fs[idx]);
                waddnstr(chan->main_win, chan->submission_fs + idx, blen);
                idx += blen;
            }
        }
        getyx(chan->main_win, y, x);
    } while (y == i);
}

void chan_draw_submissions(struct chan *chan) {
    wclear(chan->main_win);
    for (int i = 0; i < chan->nsubmissions; ++i) {
        chan_redraw_submission(chan, i);
    }
    wrefresh(chan->main_win);
}

int chan_submissions_key(struct chan *chan, int ch) {
    switch (ch) {
        case 'j':
            if (chan->active_submission < chan->nsubmissions - 1) {
                ++chan->active_submission;
            }
            chan_redraw_submission(chan, chan->active_submission - 1);
            chan_redraw_submission(chan, chan->active_submission);
            wrefresh(chan->main_win);
            return 1;
        case 'k':
            if (chan->active_submission > 0) {
                --chan->active_submission;
            }
            chan_redraw_submission(chan, chan->active_submission + 1);
            chan_redraw_submission(chan, chan->active_submission);
            wrefresh(chan->main_win);
            return 1;
        case 'o':
            urlopen(chan->submissions[chan->active_submission].url);
            return 1;
        case 'r':
            chan_update_submissions(chan);
            chan_draw_submissions(chan);
            return 1;
        case 'u': {
            char *buf = malloc(100);
            sprintf(buf, "https://news.ycombinator.com/vote?id=%d&how=up&auth=%s",
                    chan->submissions[chan->active_submission].id,
                    chan->submissions[chan->active_submission].auth);
            do_GET_nodata(chan->curl, buf);
            free(buf);
            return 1;
        }
        case '\n':
            chan->viewing = chan->submissions + chan->active_submission;
            if (!chan->viewing->comments) chan_update_comments(chan);
            chan_draw_comments(chan);
            return 1;
        default:
            return 0;
    }
}
