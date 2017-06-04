#include "submissions.h"
#include "comments.h"
#include "title.h"
#include "net.h"
#include "parse.h"
#include "sys.h"

#include <stdlib.h>
#include <string.h>

/*
 * recursive linked list free for freeing fmt pointers
 */
void free_fmt(struct fmt *fmt) {
    if (fmt) {
        free_fmt(fmt->next);
        free(fmt);
    }
}

/*
 * free all the memory and zero everything related to displayed comments
 */
void chan_com_destroy(struct chan *chan) {
    for (int i = 0; i < chan->com.lines; ++i) {
        free(chan->com.buf[i]);
        free_fmt(chan->com.buf_fmt[i]);
    }
    free(chan->com.buf);
    chan->com.buf = NULL;
    free(chan->com.buf_fmt);
    chan->com.buf_fmt = NULL;
    free(chan->com.offsets);
    chan->com.offsets = NULL;
    chan->com.lines = 0;
    chan->com.scroll = 0;
    chan->com.urlnbuf[0] = '\0';
}

/*
 * return a newly allocated buffer containing the first len bytes of src, with
 * HTML entities and tags replaced by the appropriate character(s)
 *
 * as a side effect, when encountering <a> tags, this will also add their
 * targets to chan->com.sub->urls
 */
char *unhtml(struct chan *chan, char *src, int len) {
    char *dest = malloc(len + 1);

    // i corresponds to the current position in src, j in dest
    int i, j;
    for (i = 0, j = 0; i < len;) {
        if (src[i] == '&') {
            // parse HTML entity
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
            // parse HTML tag
            if (!strncmp(src + i, "<p>", 3)) {
                i += 3;
                dest[j++] = '\n';
                dest[j++] = '\n';
            } else if (!strncmp(src + i, "<a ", 3)) {
                char *urlstart = jumpquot(src + i, 1),
                     *urlend = strchr(urlstart, '"');
                char *url = unhtml(chan, urlstart, urlend - urlstart);
                chan->com.sub->urls = realloc(chan->com.sub->urls,
                        (++chan->com.sub->nurls) * sizeof *chan->com.sub->urls);
                chan->com.sub->urls[chan->com.sub->nurls - 1] = url;

                i = jumptag(src + i, 2) - src;
                // this \x01 will be replaced with an opening bracket in
                // add_view_line, but we need to use a distinctive marker
                // so that URL formatting can be applied later
                dest[j++] = 1;
                int startidx = j;
                for (int n = chan->com.sub->nurls; n; n /= 10) {
                    for (int idx = startidx; idx < j; ++idx) {
                        dest[idx+1] = dest[idx];
                    }
                    dest[startidx] = '0' + (n % 10);
                    ++j;
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
            // no parsing to be done, copy verbatim
            dest[j++] = src[i++];
        }
    }

    dest[j] = '\0';
    return dest;
}

/*
 * copies the information relevant to chan->com.sub into corresponding fields,
 * including the data itself, links found inside of it, etc.
 */
void chan_com_update(struct chan *chan) {
    chan_com_destroy(chan);

    // we can guess at how much space we'll probably need with the comment
    // count reported from the front page, so allocate that much
    free(chan->com.sub->coms);
    chan->com.sub->coms = malloc(chan->com.sub->ncoms *
            sizeof *chan->com.sub->coms);
    struct com *coms = chan->com.sub->coms;

    // make the GET request and obtain the HTML data
    char *url = malloc(50);
    sprintf(url, "https://news.ycombinator.com/item?id=%d", chan->com.sub->id);
    char *data = http(chan->curl, url, NULL, 1);
    free(url);

    // this loop runs once for each comment found
    int idx = 0;
    while ((data = strstr(data, "<tr class='athing comtr"))) {
        // expand the buffer if necessary
        if (idx >= chan->com.sub->ncoms) {
            chan->com.sub->ncoms = idx + 1;
            chan->com.sub->coms = realloc(chan->com.sub->coms,
                    chan->com.sub->ncoms * sizeof *coms);
            coms = chan->com.sub->coms;
        }

        data = jumpapos(data, 3);
        coms[idx].id = atoi(data);

        data = jumpquot(strstr(data, "width"), 1);
        coms[idx].depth = atoi(data) / 40;

        char *auth = strstr(data, "auth=");
        coms[idx].voted = 0;
        if (auth && auth < jumpch(data, '\n', 1)) {
            copyuntil(&coms[idx].auth, auth + strlen("auth="), '&');
            if (!strncmp(jumpapos(auth, 2), "nosee", 5)) coms[idx].voted = 1;
        } else coms[idx].auth = NULL;

        data = jumptag(strstr(data, "hnuser"), 1);
        // HN colors new users in green with <font></font>
        // take that into account
        int newuser = *data == '<';
        if (newuser) data = jumptag(data, 1);
        copyuntil(&coms[idx].user, data, '<');

        data = jumptag(data, newuser ? 4 : 3);
        copyage(&coms[idx].age, data);

        data = jumpquot(strstr(data, "<span class=\"c"), 1) + 1;
        int badness = strtol(data, NULL, 16);
        coms[idx].badness = badness ? badness / 17 - 4 : 0; // :^)

        data = jumptag(data, 1);
        int text_len = strstr(data, "</span>") - data;
        coms[idx].text = unhtml(chan, data, text_len);;

        ++idx;
    }

    // if some comments were deleted since the main page was loaded, shrink
    // the buffer appropriately
    if (idx < chan->com.sub->ncoms - 1) {
        chan->com.sub->ncoms = idx + 1;
        chan->com.sub->coms = realloc(chan->com.sub->coms,
                chan->com.sub->ncoms * sizeof *coms);
    }
}

/*
 * add a single stretch of formatting to the line at the specified index
 */
void add_view_fmt_at(struct chan *chan, int type, int offset, int len, int idx) {
    struct fmt *fmt = malloc(sizeof *fmt);
    fmt->type = type;
    fmt->offset = offset;
    fmt->len = len;
    fmt->next = NULL;

    if (chan->com.buf_fmt[idx]) {
        // there was already formatting, append to the linked list
        struct fmt *node = chan->com.buf_fmt[idx];
        while (node->next) node = node->next;
        node->next = fmt;
    } else {
        // make the linked list
        chan->com.buf_fmt[idx] = fmt;
    }
}

/*
 * same as above, but do it at the line most recently added
 */
void add_view_fmt(struct chan *chan, int type, int offset, int len) {
    add_view_fmt_at(chan, type, offset, len, chan->com.lines - 1);
}

/*
 * generate a line for display
 *
 * mostly just applies the indent requested
 */
char *make_view_line(struct chan *chan, char *str, int len, int indent) {
    char *line = malloc(len + indent + 1);
    memset(line, ' ', indent);
    strncpy(line + indent, str, len);
    line[len + indent] = '\0';
    return line;
}

/*
 * add a line to the display buffer
 */
void add_view_line(struct chan *chan, char *str, int len, int indent) {
    char *line = make_view_line(chan, str, len, indent);

    // allocate space in each buffer and place the appropriate values into it
    chan->com.buf = realloc(chan->com.buf,
            (++chan->com.lines) * sizeof *chan->com.buf);
    chan->com.buf[chan->com.lines - 1] = line;

    chan->com.buf_fmt = realloc(chan->com.buf_fmt,
            chan->com.lines * sizeof *chan->com.buf_fmt);
    chan->com.buf_fmt[chan->com.lines - 1] = NULL;

    // fix \x01 markers for URLs and add their formatting
    for (int i = 0; line[i]; ++i) {
        if (line[i] == 1) {
            line[i] = '[';
            add_view_fmt(chan, FMT_URL, i, strchr(line + i, ']') - (line + i) + 1);
        }
    }
}

// a comment consists of all the lines [start,stop)
#define OFFSET_START(idx) (chan->com.offsets[(idx)])
#define OFFSET_STOP(idx) (((idx) == chan->com.sub->ncoms - 1 ? \
        chan->com.lines : chan->com.offsets[(idx) + 1]) - 1)

// top and bottom of the displayed screen
#define VIEW_TOP (chan->com.scroll)
#define VIEW_BOTTOM (chan->com.scroll + chan->main_lines - 1)

/*
 * draw a single line from the display buffer
 *
 * more specifically, at physical screen position 'y', draw line 'lineno'
 *
 * this does NOT call wrefresh()!
 */
void draw_view_line(struct chan *chan, int lineno) {
    int y = lineno - VIEW_TOP;
    char *line = chan->com.buf[lineno];
    struct fmt *fmt = chan->com.buf_fmt[lineno];
    int idx = 0;

    // clear anything that might be already on this line
    // this happens e.g. when redrawing headers after a vote
    wmove(chan->main_win, y, 0);
    wclrtoeol(chan->main_win);

    // draw everything up to and including the last formatting stretch
    while (fmt) {
        wattron(chan->main_win, COLOR_PAIR(PAIR_WHITE));
        wattrset(chan->main_win, 0);
        mvwaddnstr(chan->main_win, y, idx, line + idx, fmt->offset - idx);
        switch (fmt->type) {
            case FMT_USER: wattron(chan->main_win, A_BOLD | COLOR_PAIR(PAIR_MAGENTA)); break;
            case FMT_AGE: wattron(chan->main_win, A_BOLD | COLOR_PAIR(PAIR_YELLOW)); break;
            case FMT_URL: wattron(chan->main_win, A_BOLD | COLOR_PAIR(PAIR_BLUE)); break;
            case FMT_UP: wattron(chan->main_win, A_BOLD | COLOR_PAIR(PAIR_GREEN_BG)); break;
            case FMT_BAD: wattron(chan->main_win, A_BOLD | COLOR_PAIR(PAIR_RED_BG)); break;
        }
        mvwaddnstr(chan->main_win, y, fmt->offset, line + fmt->offset, fmt->len);

        idx = fmt->offset + fmt->len;
        fmt = fmt->next;
    }

    // draw the last section of normal unformatted text
    wattron(chan->main_win, COLOR_PAIR(PAIR_WHITE));
    wattrset(chan->main_win, 0);
    mvwaddstr(chan->main_win, y, idx, line + idx);

    // draw the column representing the active comment, if applicable
    if (lineno >= OFFSET_START(chan->com.active) &&
            lineno < OFFSET_STOP(chan->com.active)) {
        int colpos = chan->com.sub->coms[chan->com.active].depth * 2;
        wattron(chan->main_win, COLOR_PAIR(PAIR_CYAN_BG));
        mvwaddch(chan->main_win, y, colpos, ' ');
    }
}

/*
 * render a comment header
 *
 * can be used for both the initial creation (i.e. during the process of
 * building chan->com.buf) and updating the header later (e.g. when voting on a
 * comment)
 */
void render_header(struct chan *chan, int idx) {
    struct com com = chan->com.sub->coms[idx];
    int indent = com.depth * 2 + 1, linewidth = chan->main_cols - indent,
        lineno = chan->com.offsets[idx];

    // build the string
    char *head = malloc(linewidth + 1);
    snprintf(head, linewidth + 1, "%s [%s]", com.user, com.age);
    if (com.voted) {
        int hlen = strlen(head);
        strncat(head, " +", linewidth - hlen);
    }
    if (com.badness) {
        int hlen = strlen(head);
        strncat(head, "  -x ", linewidth - hlen);
        if (hlen + 3 < linewidth) head[hlen + 3] = '0' + com.badness;
    }

    // append it if this is the first time we're generating the header;
    // otherwise, modify the existing line
    if (lineno == chan->com.lines) {
        add_view_line(chan, head, linewidth, indent);
    } else {
        free(chan->com.buf[lineno]);
        chan->com.buf[lineno] = make_view_line(chan, head, linewidth, indent);

        // also clear the formatting so we can regenerate it
        free_fmt(chan->com.buf_fmt[lineno]);
        chan->com.buf_fmt[lineno] = NULL;
    }

    int ulen = strlen(com.user), alen = strlen(com.age);
    add_view_fmt_at(chan, FMT_USER, indent, ulen, lineno);
    add_view_fmt_at(chan, FMT_AGE, indent + ulen + 2, alen, lineno);
    if (com.voted) {
        add_view_fmt_at(chan, FMT_UP, indent + ulen + 2 + alen + 2, 1, lineno);
    }
    if (com.badness) {
        add_view_fmt_at(chan, FMT_BAD, indent + ulen + 2 + alen + 2 +
                (com.voted ? 2 : 0), 4, lineno);
    }

    free(head);
}

/*
 * what happens when you ^L
 * (TODO)
 */
void chan_com_refresh(struct chan *chan) {
    chan_title_refresh(chan);
    wclear(chan->main_win);
    for (int i = chan->com.scroll;
            i < chan->com.lines && i < chan->com.scroll + chan->main_lines;
            ++i) {
        draw_view_line(chan, i);
    }
    wrefresh(chan->main_win);
    wclear(chan->status_win);
    wrefresh(chan->status_win);
}

/*
 * render all parsed comments into the view buffer and draw the visible top
 * section
 */
void chan_com_draw(struct chan *chan) {
    wclear(chan->main_win);
    scrollok(chan->main_win, TRUE);

    // allocate space for linewise offsets of each comment
    free(chan->com.offsets);
    chan->com.offsets = malloc(chan->com.sub->ncoms *
            sizeof *chan->com.offsets);

    // TODO unicode support
    for (int i = 0; i < chan->com.sub->ncoms; ++i) {
        struct com com = chan->com.sub->coms[i];
        int lastspace = 0, breakidx = 0, indent = com.depth * 2 + 1,
            linewidth = chan->main_cols - indent;

        // save the position/line on which the comment starts
        chan->com.offsets[i] = chan->com.lines;

        // create and output a header
        render_header(chan, i);

        // time to word wrap!
        for (int j = 0; j < strlen(com.text); ++j) {
            if (j - breakidx >= linewidth) {
                // there are too many characters since we last broke a line
                // so we have to break a line now
                if (lastspace) {
                    // there has been a space; break at the most recent one
                    add_view_line(chan, com.text + breakidx, lastspace, indent);
                    breakidx += lastspace + 1;
                    lastspace = 0;
                } else {
                    // there hasn't, so just cut the giant word in half
                    add_view_line(chan, com.text + breakidx, linewidth, indent);
                    breakidx += linewidth;
                }
            } else if (com.text[j] == ' ') {
                // store the position of the most recent space for breaking
                lastspace = j - breakidx;
            } else if (com.text[j] == '\n') {
                // newlines force a line break
                add_view_line(chan, com.text + breakidx, j - breakidx, indent);
                lastspace = 0;
                breakidx = j + 1;
            }
        }

        // add the remaining (last) line
        add_view_line(chan, com.text + breakidx, linewidth, indent);
        add_view_line(chan, "", 0, 0);
    }

    // draw the initially visible part of the display buffer
    // (we assume that we're starting at the top)
    chan->com.scroll = 0;
    chan->com.active = 0;

    chan_com_refresh(chan);
}

/*
 * scroll the view by 'amount' lines (can be negative to scroll up)
 *
 * scroll_view_abs is similar, but not relative and sets the scroll position
 *
 * WARNING: THIS DOES NOT DO ANY BOUNDS CHECKS!!!
 */
void scroll_view(struct chan *chan, int amount) {
    chan->com.scroll += amount;
    wscrl(chan->main_win, amount);
    if (amount > 0) {
        // scrolling down, draw new lines at the bottom
        for (int i = 1; i <= amount; ++i) {
            draw_view_line(chan, VIEW_BOTTOM - i + 1);
        }
    } else {
        // scrolling up, draw new lines at the top
        for (int i = 1; i <= -amount; ++i) {
            draw_view_line(chan, VIEW_TOP + i - 1);
        }
    }
}
#define scroll_view_abs(chan,x) scroll_view((chan), (x) - chan->com.scroll)

/*
 * this takes a comment index and either clears or draws the column indicating
 * an active comment
 *
 * should be called, when the active comment changes, on any comments that were
 * affected (the new active comment, and the old one if it's still on screen)
 */
void redraw_active_col(struct chan *chan, int idx) {
    // either clear or draw the active indicator depending on which comment
    // the redraw is requested on
    wattron(chan->main_win, COLOR_PAIR(idx == chan->com.active ?
                PAIR_CYAN_BG : PAIR_WHITE));
    wattrset(chan->main_win, 0);

    // find the bounds on the comment itself
    int start = OFFSET_START(idx);
    int stop = OFFSET_STOP(idx);

    // translate to screen coordinates
    start -= VIEW_TOP;
    stop -= VIEW_TOP;

    // clamp the bounds such that they're on the screen
    start = start < 0 ? 0 : start;
    stop = stop > chan->main_lines ? chan->main_lines : stop;

    // actually draw the line
    for (int i = start; i < stop; ++i) {
        mvwaddch(chan->main_win, i, chan->com.sub->coms[idx].depth * 2, ' ');
    }

    // be nice to later ncurses calls
    wattron(chan->main_win, COLOR_PAIR(PAIR_WHITE));
}

/*
 * given a comment index, make the comment at that index active
 */
void set_active_comment(struct chan *chan, int i) {
    int old_active = chan->com.active;
    chan->com.active = i;

    if (i > old_active) {
        // get as much of the newly focused comment on screen as
        // possible (ideally, all of it)
        if (OFFSET_STOP(chan->com.active) > VIEW_BOTTOM) {
            int new_scroll = OFFSET_STOP(chan->com.active) -
                chan->main_lines;
            // don't scroll past the beginning, though
            if (new_scroll > OFFSET_START(chan->com.active)) {
                new_scroll = OFFSET_START(chan->com.active);
            }
            scroll_view_abs(chan, new_scroll);
        }
    } else {
        // same deal as with above
        if (OFFSET_START(chan->com.active) < VIEW_TOP) {
            scroll_view_abs(chan, OFFSET_START(chan->com.active));
        }
    }

    redraw_active_col(chan, old_active);
    redraw_active_col(chan, chan->com.active);
    wrefresh(chan->main_win);
}

/*
 * called on every keypress while viewing comments
 */
#define ACTIVE chan->com.sub->coms[chan->com.active]
int chan_com_key(struct chan *chan, int ch) {
    if ((ch >= '0' && ch <= '9') || ch == '\x7f') {
        // a key that involves the URL number buffer
        wclear(chan->status_win);

        int len = strlen(chan->com.urlnbuf);
        if (ch == '\x7f') {
            // backspace
            if (len) chan->com.urlnbuf[len-1] = '\0';
        } else {
            // append to buffer
            chan->com.urlnbuf[len] = ch;
            chan->com.urlnbuf[len+1] = '\0';
        }

        int urln = atoi(chan->com.urlnbuf);
        if (!urln || urln > chan->com.sub->nurls) {
            // invalid number, so clear the buffer
            chan->com.urlnbuf[0] = '\0';
        } else {
            // display the url corresponding to the number
            char *statusbuf = malloc(COLS + 1);
            snprintf(statusbuf, COLS + 1, "[%s] %s", chan->com.urlnbuf,
                    chan->com.sub->urls[urln - 1]);
            mvwaddstr(chan->status_win, 0, 0, statusbuf);
            free(statusbuf);
        }
        wrefresh(chan->status_win);

        return 1;
    }

    else if (ch == '\x1b') {
        chan->com.urlnbuf[0] = '\0';
        wclear(chan->status_win);
        wrefresh(chan->status_win);
        return 1;
    }

    else if (ch == chan->keys.com_scroll_down) {
        if (VIEW_BOTTOM < chan->com.lines - 1) {
            scroll_view(chan, 1);

            // check to see whether we just scrolled the active comment
            // out of view
            if (OFFSET_STOP(chan->com.active) <= VIEW_TOP) {
                redraw_active_col(chan, ++chan->com.active);
            }

            wrefresh(chan->main_win);
        }
        return 1;
    }

    else if (ch == chan->keys.com_scroll_up) {
        if (VIEW_TOP > 0) {
            scroll_view(chan, -1);

            // as in 'j', check to see whether the active comment was
            // scrolled away
            if (OFFSET_START(chan->com.active) > VIEW_BOTTOM) {
                redraw_active_col(chan, --chan->com.active);
            }
            wrefresh(chan->main_win);
        }
        return 1;
    }

    else if (ch == chan->keys.com_next) {
        if (chan->com.active < chan->com.sub->ncoms - 1) {
            set_active_comment(chan, chan->com.active + 1);
        }
        return 1;
    }

    else if (ch == chan->keys.com_prev) {
        if (chan->com.active > 0) {
            set_active_comment(chan, chan->com.active - 1);
        }
        return 1;
    }

    else if (ch == chan->keys.com_next_at_depth) {
        for (int d = ACTIVE.depth, i = chan->com.active + 1;
                i < chan->com.sub->ncoms; ++i) {
            if (chan->com.sub->coms[i].depth == d) {
                set_active_comment(chan, i);
                break;
            } else if (chan->com.sub->coms[i].depth < d) {
                break;
            }
        }
        return 1;
    }

    else if (ch == chan->keys.com_prev_at_depth) {
        for (int d = ACTIVE.depth, i = chan->com.active - 1; i >= 0; --i) {
            if (chan->com.sub->coms[i].depth == d) {
                set_active_comment(chan, i);
                break;
            } else if (chan->com.sub->coms[i].depth < d) {
                break;
            }
        }
        return 1;
    }

    else if (ch == chan->keys.com_open_url) {
        if (chan->com.urlnbuf[0]) {
            wclear(chan->status_win);
            wrefresh(chan->status_win);
            urlopen(chan->com.sub->urls[atoi(chan->com.urlnbuf)-1]);
            chan->com.urlnbuf[0] = '\0';
        } else urlopen(chan->com.sub->url);
        return 1;
    }

    else if (ch == chan->keys.com_back) {
        chan->com.sub = NULL;
        chan_com_destroy(chan);

        wclear(chan->status_win);
        wrefresh(chan->status_win);

        chan_sub_draw(chan);

        return 1;
    }

    else if (ch == chan->keys.com_upvote) {
        if (chan->authenticated) {
            char *buf = malloc(100);
            sprintf(buf, "https://news.ycombinator.com/vote?id=%d&how=u%c&auth=%s",
                    ACTIVE.id, ACTIVE.voted ? 'n' : 'p', ACTIVE.auth);
            http(chan->curl, buf, NULL, 0);
            free(buf);

            ACTIVE.voted = 1 - ACTIVE.voted;
            render_header(chan, chan->com.active);
            draw_view_line(chan, chan->com.offsets[chan->com.active]);
            wrefresh(chan->main_win);
        } else {
            wclear(chan->status_win);
            mvwaddstr(chan->status_win, 0, 0,
                    "You must be authenticated to do that.");
            wrefresh(chan->status_win);
        }
        return 1;
    }

    else {
        return 0;
    }
}
