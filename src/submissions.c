#include "submissions.h"
#include "comments.h"
#include "net.h"
#include "parse.h"

#include <stdlib.h>
#include <string.h>

void chan_destroy_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        free(chan->submissions[i].url);
        free(chan->submissions[i].title);
        free(chan->submissions[i].user);
        free(chan->submissions[i].comments);
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
    char *line = malloc(COLS + 1);
    int written;
    if (submission.job) {
        written = snprintf(line, COLS + 1, "    %3s     %s", submission.age, submission.title);
    } else {
        written = snprintf(line, COLS + 1, "%3d %3s %3d %s", submission.score, submission.age, submission.ncomments, submission.title);
    }
    if (written < COLS) {
        memset(line + written, ' ', COLS - written);
        line[COLS] = '\0';
    }
    mvwaddstr(chan->main_win, i, 0, line);
    free(line);
}

void chan_draw_submissions(struct chan *chan) {
    wclear(chan->main_win);
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
            free(url);
            free(cmd);
            break;
        }
        case '\n':
            chan->viewing = chan->submissions + chan->active_submission;
            if (!chan->viewing->comments) chan_update_comments(chan);
            chan_draw_comments(chan);
            break;
    }
}
