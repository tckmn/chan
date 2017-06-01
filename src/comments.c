#include "submissions.h"
#include "comments.h"
#include "net.h"
#include "parse.h"

#include <stdlib.h>
#include <string.h>

char *unhtml(struct chan *chan, char *src, int len) {
    char *dest = malloc(len + 1);
    int i, j;
    for (i = 0, j = 0; i < len;) {
        if (src[i] == '&') {
            if (!strncmp(src + i, "&#x27;", 6)) {
                i += 6;
                dest[j++] = '\'';
            } else if (!strncmp(src + i, "&#x2F;", 6)) {
                i += 6;
                dest[j++] = '/';
            } else if (!strncmp(src + i, "&quot;", 6)) {
                i += 6;
                dest[j++] = '"';
            } else if (!strncmp(src + i, "&lt;", 4)) {
                i += 4;
                dest[j++] = '<';
            } else if (!strncmp(src + i, "&gt;", 4)) {
                i += 4;
                dest[j++] = '>';
            } else if (!strncmp(src + i, "&amp;", 5)) {
                i += 5;
                dest[j++] = '&';
            } else ++i;
        } else if (src[i] == '<') {
            if (!strncmp(src + i, "<p>", 3)) {
                i += 3;
                dest[j++] = '\n';
                dest[j++] = '\n';
            } else if (!strncmp(src + i, "<a ", 3)) {
                char *urlstart = jumpquot(src + i, 1),
                     *urlend = strchr(urlstart, '"');
                char *url = unhtml(chan, urlstart, urlend - urlstart);
                chan->viewing->urls = realloc(chan->viewing->urls,
                        (++chan->viewing->nurls) * sizeof *chan->viewing->urls);
                chan->viewing->urls[chan->viewing->nurls - 1] = url;

                i = jumptag(src + i, 2) - src;
                // this \x01 will be replaced with an opening bracket in
                // add_view_line, but we need to use a distinctive marker
                // so that URL formatting can be applied later
                dest[j++] = 1;
                for (int n = chan->viewing->nurls; n; n /= 10) {
                    dest[j++] = '0' + (n % 10);
                }
                dest[j++] = ']';
            } else if (!strncmp(src + i, "<i>", 3)) {
                i += 3;
                dest[j++] = '*';
            } else if (!strncmp(src + i, "</i>", 4)) {
                i += 4;
                dest[j++] = '*';
            } else if (!strncmp(src + i, "<pre><code>", 11)) {
                i += 11;
            } else if (!strncmp(src + i, "</code></pre>", 13)) {
                i += 13;
            } else if (!strncmp(src + i, "<span>", 6)) {
                // HN has broken html and renders its comments as
                // <span class="c##"> ... <span></span>
                // so I guess we handle broken html with broken methods
                break;
            } else ++i;
        } else {
            dest[j++] = src[i++];
        }
    }
    dest[j] = '\0';
    return dest;
}

void chan_update_comments(struct chan *chan) {
    free(chan->viewing->comments);
    chan->viewing->comments = malloc(chan->viewing->ncomments *
            sizeof *chan->viewing->comments);
    struct comment *comments = chan->viewing->comments;

    char *url = malloc(50);
    sprintf(url, "https://news.ycombinator.com/item?id=%d", chan->viewing->id);
    char *data = do_GET(chan->curl, url);
    free(url);

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
        int text_len = strstr(data, "</span>") - data;
        comments[idx].text = unhtml(chan, data, text_len);;

        ++idx;
    }
    if (idx < chan->viewing->ncomments - 1) {
        chan->viewing->ncomments = idx + 1;
        chan->viewing->comments = realloc(chan->viewing->comments,
                chan->viewing->ncomments * sizeof *comments);
    }
}

void add_view_fmt(struct chan *chan, int type, int offset, int len) {
    struct fmt *fmt = malloc(sizeof *fmt);
    fmt->type = type;
    fmt->offset = offset;
    fmt->len = len;
    fmt->next = NULL;

    if (chan->view_buf_fmt[chan->view_lines - 1]) {
        struct fmt *node = chan->view_buf_fmt[chan->view_lines - 1];
        while (node->next) node = node->next;
        node->next = fmt;
    } else {
        chan->view_buf_fmt[chan->view_lines - 1] = fmt;
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

    chan->view_buf_fmt = realloc(chan->view_buf_fmt,
            chan->view_lines * sizeof *chan->view_buf_fmt);
    chan->view_buf_fmt[chan->view_lines - 1] = NULL;

    // fix \x01 markers for URLs and add their formatting
    for (int i = 0; line[i]; ++i) {
        if (line[i] == 1) {
            line[i] = '[';
            add_view_fmt(chan, FMT_URL, i, strchr(line + i, ']') - (line + i) + 1);
        }
    }
}

void draw_view_line(struct chan *chan, int y, int lineno) {
    char *line = chan->view_buf[lineno];
    struct fmt *fmt = chan->view_buf_fmt[lineno];
    int idx = 0;

    while (fmt) {
        wattrset(chan->main_win, 0);
        mvwaddnstr(chan->main_win, y, idx, line + idx, fmt->offset - idx);
        switch (fmt->type) {
            case FMT_USER: wattrset(chan->main_win, A_BOLD | COLOR_PAIR(1)); break;
            case FMT_AGE: wattrset(chan->main_win, A_BOLD | COLOR_PAIR(2)); break;
            case FMT_URL: wattrset(chan->main_win, A_BOLD | COLOR_PAIR(3)); break;
        }
        mvwaddnstr(chan->main_win, y, fmt->offset, line + fmt->offset, fmt->len);

        idx = fmt->offset + fmt->len;
        fmt = fmt->next;
    }
    wattrset(chan->main_win, 0);
    mvwaddstr(chan->main_win, y, idx, line + idx);
}

void chan_draw_comments(struct chan *chan) {
    wclear(chan->main_win);

    // TODO unicode support
    for (int i = 0; i < chan->viewing->ncomments; ++i) {
        struct comment comment = chan->viewing->comments[i];
        int lastspace = 0, breakidx = 0, indent = comment.depth * 2,
            linewidth = chan->main_cols - indent;

        char *head = malloc(linewidth + 1);
        snprintf(head, linewidth + 1, "%s [%s]", comment.user, comment.age);
        add_view_line(chan, head, linewidth, indent);
        add_view_fmt(chan, FMT_USER, indent, strlen(comment.user));
        add_view_fmt(chan, FMT_AGE, indent + strlen(comment.user) + 2, strlen(comment.age));
        free(head);

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

    chan->view_scroll = 0;
    for (int i = 0; i < chan->view_lines && i < chan->main_lines; ++i) {
        draw_view_line(chan, i, i);
    }

    wrefresh(chan->main_win);
}

void chan_comments_key(struct chan *chan, int ch) {
    if (ch >= '0' && ch <= '9') {
        wclear(chan->status_win);

        int len = strlen(chan->view_urlnbuf);
        chan->view_urlnbuf[len] = ch;
        chan->view_urlnbuf[len+1] = '\0';

        int urln = atoi(chan->view_urlnbuf);
        if (!urln || urln > chan->viewing->nurls) {
            chan->view_urlnbuf[0] = '\0';
        } else {
            char *statusbuf = malloc(COLS + 1);
            snprintf(statusbuf, COLS + 1, "[%s] %s", chan->view_urlnbuf,
                    chan->viewing->urls[urln - 1]);
            mvwaddstr(chan->status_win, 0, 0, statusbuf);
            free(statusbuf);
        }
        wrefresh(chan->status_win);
    } else switch (ch) {
        case 'j':
            if (chan->view_scroll + chan->main_lines < chan->view_lines) {
                ++chan->view_scroll;
                wscrl(chan->main_win, 1);
                draw_view_line(chan, chan->main_lines - 1,
                        chan->view_scroll + chan->main_lines);
                wrefresh(chan->main_win);
            }
            break;
        case 'k':
            if (chan->view_scroll > 0) {
                --chan->view_scroll;
                wscrl(chan->main_win, -1);
                draw_view_line(chan, 0, chan->view_scroll);
                wrefresh(chan->main_win);
            }
            break;
        case 'o': {
            // TODO do this in a better way
            // TODO don't duplicate code from submissions.c
            char *url = chan->viewing->url;
            char *cmd = malloc(strlen(url) + 12);
            sprintf(cmd, "xdg-open '%s'", url);
            system(cmd);
            free(url);
            free(cmd);
            break;
        }
        case 'q':
            chan->viewing = NULL;
            for (int i = 0; i < chan->view_lines; ++i) {
                free(chan->view_buf[i]);
                free(chan->view_buf_fmt[i]);
            }
            free(chan->view_buf);
            chan->view_buf = NULL;
            free(chan->view_buf_fmt);
            chan->view_buf_fmt = NULL;
            chan->view_lines = 0;
            chan->view_scroll = 0;
            chan->view_urlnbuf[0] = '\0';
            wclear(chan->status_win);
            wrefresh(chan->status_win);
            chan_draw_submissions(chan);
            break;
    }
}
