#include "submissions.h"
#include "comments.h"
#include "net.h"
#include "parse.h"

#include <stdlib.h>
#include <string.h>

void chan_update_comments(struct chan *chan) {
    free(chan->viewing->comments);
    chan->viewing->comments = malloc(chan->viewing->ncomments *
            sizeof *chan->viewing->comments);
    struct comment *comments = chan->viewing->comments;

    char *url = malloc(50);
    sprintf(url, "https://news.ycombinator.com/item?id=%d", chan->viewing->id);
    char *data = do_GET(chan->curl, url);

    int idx = 0;
    while ((data = strstr(data, "<tr class='athing comtr"))) {
        if (idx >= chan->viewing->ncomments) {
            chan->viewing->ncomments = idx + 1;
            chan->viewing->comments = realloc(chan->viewing->comments,
                    chan->viewing->ncomments * sizeof *comments);
            comments = chan->viewing->comments;
        }

        data = jumpapos(data, 3);
        comments[idx].id = atoi(data);

        data = jumpquot(strstr(data, "width"), 1);
        comments[idx].depth = atoi(data) / 40;

        data = jumptag(strstr(data, "hnuser"), 1);
        copyuntil(&comments[idx].user, data, '<');

        data = jumptag(data, 3);
        copyage(&comments[idx].age, data);

        data = jumpquot(strstr(data, "<span class=\"c"), 1) + 1;
        int badness = strtol(data, NULL, 16);
        comments[idx].badness = badness ? badness / 17 - 4 : 0;

        data = jumptag(data, 1);
        int text_len = strchr(data, '\n') - data - 6;
        char *text = malloc(text_len + 1);
        int i, j;
        for (i = 0, j = 0; i < text_len;) {
            if (data[i] == '&') {
                if (!strncmp(data + i, "&#x27;", 6)) {
                    i += 6;
                    text[j++] = '\'';
                } else if (!strncmp(data + i, "&#x2F;", 6)) {
                    i += 6;
                    text[j++] = '/';
                } else if (!strncmp(data + i, "&quot;", 6)) {
                    i += 6;
                    text[j++] = '"';
                } else if (!strncmp(data + i, "&lt;", 4)) {
                    i += 4;
                    text[j++] = '<';
                } else if (!strncmp(data + i, "&gt;", 4)) {
                    i += 4;
                    text[j++] = '>';
                } else ++i;
            } else if (data[i] == '<') {
                if (!strncmp(data + i, "<p>", 3)) {
                    i += 3;
                    text[j++] = '\n';
                    text[j++] = '\n';
                } else if (!strncmp(data + i, "<a ", 3)) {
                    // TODO do something with links, make them clickable
                    i += strchr(data + i, '>') - (data + i) + 1;
                } else if (!strncmp(data + i, "</a>", 4)) {
                    i += 4;
                } else if (!strncmp(data + i, "<i>", 3)) {
                    i += 3;
                    text[j++] = '*';
                } else if (!strncmp(data + i, "</i>", 4)) {
                    i += 4;
                    text[j++] = '*';
                } else ++i;
            } else {
                text[j++] = data[i++];
            }
        }
        text[j] = '\0';
        comments[idx].text = text;

        ++idx;
    }
}

void add_view_line(struct chan *chan, char *str, int len, int indent) {
    char *line = malloc(len + indent + 1);
    memset(line, ' ', indent);
    strncpy(line + indent, str, len);
    line[len + indent] = '\0';

    chan->view_buf = realloc(chan->view_buf,
            (++chan->view_lines) * sizeof *chan->view_buf);
    chan->view_buf[chan->view_lines - 1] = line;
}

void chan_draw_comments(struct chan *chan) {
    wclear(chan->main_win);
    wattrset(chan->main_win, 0);

    // TODO unicode support
    for (int i = 0; i < chan->viewing->ncomments; ++i) {
        struct comment comment = chan->viewing->comments[i];
        int lastspace = 0, breakidx = 0, indent = comment.depth * 2,
            linewidth = COLS - indent;
        for (int j = 0; j < strlen(comment.text); ++j) {
            if (j - breakidx >= linewidth) {
                if (lastspace) {
                    add_view_line(chan, comment.text + breakidx, lastspace, indent);
                    breakidx += lastspace + 1;
                    lastspace = 0;
                } else {
                    add_view_line(chan, comment.text + breakidx, linewidth, indent);
                    breakidx += linewidth;
                }
            } else if (comment.text[j] == ' ') {
                lastspace = j - breakidx;
            } else if (comment.text[j] == '\n') {
                add_view_line(chan, comment.text + breakidx, j - breakidx, indent);
                lastspace = 0;
                breakidx = j + 1;
            }
        }
        add_view_line(chan, comment.text + breakidx, linewidth, indent);
        add_view_line(chan, "", 0, 0);
    }

    for (int i = 0; i < chan->view_lines && i < LINES; ++i) {
        mvwaddstr(chan->main_win, i, 0, chan->view_buf[i]);
    }

    wrefresh(chan->main_win);
}

void chan_comments_key(struct chan *chan, int ch) {
    switch (ch) {
        case 'o': {
            // TODO do this in a better way
            // TODO don't duplicate code from submissions.c
            char *url = chan->viewing->url;
            char *cmd = malloc(strlen(url) + 12);
            sprintf(cmd, "xdg-open '%s'", url);
            system(cmd);
            break;
        }
        case 'q':
            chan->viewing = NULL;
            for (int i = 0; i < chan->view_lines; ++i) {
                free(chan->view_buf[i]);
            }
            free(chan->view_buf);
            chan->view_buf = NULL;
            chan->view_lines = 0;
            chan_draw_submissions(chan);
            break;
    }
}
