#include "submissions.h"
#include "comments.h"
#include "login.h"
#include "title.h"
#include "net.h"
#include "parse.h"
#include "sys.h"
#include "utf8.h"

#include <stdlib.h>
#include <string.h>

/*
 * free all the memory and zero everything related to cached submissions
 */
void chan_sub_destroy(struct chan *chan) {
    for (int i = 0; i < chan->sub.nsubs; ++i) {
        free(chan->sub.subs[i].url);
        free(chan->sub.subs[i].title);
        free(chan->sub.subs[i].user);
        free(chan->sub.subs[i].coms);
        for (int j = 0; j < chan->sub.subs[i].nurls; ++j) {
            free(chan->sub.subs[i].urls[j]);
        }
        free(chan->sub.subs[i].urls);
    }
    free(chan->sub.subs);
    chan->sub.subs = NULL;
    chan->sub.nsubs = 0;
    chan->sub.active = 0;
    chan->sub.page = 0;
}

/*
 * loads the top 30 from the front page and populates all submission-related
 * fields
 */
void chan_sub_update(struct chan *chan) {
    chan_sub_destroy(chan);
    chan->sub.subs = malloc(30 * sizeof *chan->sub.subs);
    chan->sub.nsubs = 30;

    // make GET request and obtain data
    const static char* urls[] = {
        "https://news.ycombinator.com/",
        "https://news.ycombinator.com/newest",
        "https://news.ycombinator.com/show",
        "https://news.ycombinator.com/ask",
        "https://news.ycombinator.com/jobs"
    };
    char *data = http(chan->curl, urls[chan->sub.mode], NULL, 1);

    // this loop runs once for each submission found
    struct sub *sub = chan->sub.subs;
    while ((data = strstr(data, "<tr class='athing'"))) {
        data += strlen("<tr class='athing' id='");
        sub->id = atoi(data);

        char *auth = strstr(data, "auth=");
        sub->voted = 0;
        if (auth && auth < jumpch(data, '\n', 2)) {
            copyuntil(&sub->auth, auth + strlen("auth="), '&');
            if (!strncmp(jumpapos(auth, 2), "nosee", 5)) sub->voted = 1;
        } else sub->auth = NULL;

        data = strstr(data, "<td class=\"title\"><a href=\"") +
            strlen("<td class=\"title\"><a href=\"");
        copyuntil(&sub->url, data, '"');

        data = jumptag(data, 1);
        copyuntil(&sub->title, data, '<');

        // next line always starts with <span class="
        data = strchr(strchr(data, '\n'), '=') + 2;
        if (*data == 's') {
            // "score" i.e. it's a regular sub
            sub->job = 0;

            data = jumptag(data, 1);
            sub->score = atoi(data);

            data = jumptag(data, 2);
            // HN colors new users in green with <font></font>
            // take that into account
            int newuser = *data == '<';
            if (newuser) data = jumptag(data, 1);
            copyuntil(&sub->user, data, '<');

            data = jumptag(data, newuser ? 4 : 3);
        } else if (*data == 'a') {
            // "age" i.e. it's one of those job posts
            sub->job = 1;

            sub->score = 0;

            sub->user = malloc(1);
            sub->user[0] = '\0';

            data = jumptag(data, 2);
        } else exit(123); // TODO something better
        copyage(&sub->age, data);

        sub->coms = NULL;
        if (sub->job) {
            sub->ncoms = 0;
        } else {
            data = jumptag(strstr(data, "<a href=\"item?"), 1);
            sub->ncoms = atoi(data);
        }

        ++sub;
    }
}

/*
 * redraw a single submission
 *
 * this does NOT call wrefresh()!
 */
void chan_redraw_submission(struct chan *chan, int i) {
    // if it's the active one, enable the reverse attribute
    int normal_attr = i == chan->sub.active ? A_REVERSE : 0;
    wattrset(chan->main_win, normal_attr);
    struct sub sub = chan->sub.subs[i];

    // physical screen position of this submission
    int ypos = i - chan->main_lines * chan->sub.page;

    // loop that steps through the format string specified
    int y, x, err = OK;
    char *substr = NULL;
    int idx = 0, subidx = 0;
    wmove(chan->main_win, ypos, 0);
    do {
        if (substr) {
            // we're being asked to render a given char* verbatim, so do so
            int blen = bytelen(substr[subidx]);
            err = waddnstr(chan->main_win, substr + subidx, blen);
            if (!substr[subidx += blen]) {
                // done - free memory, reset attributes
                free(substr);
                substr = NULL;
                subidx = 0;
                wattron(chan->main_win, COLOR_PAIR(PAIR_WHITE));
                wattrset(chan->main_win, normal_attr);
            }
        } else {
            // keep stepping through the format string
            if (!chan->sub.fmt_str[idx]) {
                // format string has ended, output spaces until EOL
                // (we need to for the active highlight to go all the way)
                err = waddch(chan->main_win, ' ');
            } else if (chan->sub.fmt_str[idx] == '%') {
                // format specifier, get the next char to figure out what it is
                switch (chan->sub.fmt_str[++idx]) {
                    case 's': // score
                        substr = malloc(5);
                        if (sub.job) {
                            strcpy(substr, "    ");
                        } else {
                            snprintf(substr, 5, "%4d", sub.score);
                            if (sub.voted) {
                                // highlight voted-on posts' scores in green
                                wattron(chan->main_win,
                                        A_BOLD | COLOR_PAIR(PAIR_GREEN));
                            }
                        }
                        break;
                    case 'a': // age
                        substr = malloc(4);
                        snprintf(substr, 4, "%3s", sub.age);
                        break;
                    case 'c': // comment count
                        substr = malloc(4);
                        if (sub.job) {
                            strcpy(substr, "   ");
                        } else {
                            snprintf(substr, 4, "%3d", sub.ncoms);
                        }
                        break;
                    case 't': // title
                        substr = malloc(strlen(sub.title) + 1);
                        strcpy(substr, sub.title);
                        break;
                    case '%': // literal %
                        err = waddch(chan->main_win, '%');
                        break;
                    default: // unrecognized, pretend the % wasn't special
                        err = waddch(chan->main_win, '%');
                        --idx;
                        break;
                }
                ++idx;
            } else {
                // no % in the format string, copy down the next char
                int blen = bytelen(chan->sub.fmt_str[idx]);
                err = waddnstr(chan->main_win, chan->sub.fmt_str + idx, blen);
                idx += blen;
            }
        }
        getyx(chan->main_win, y, x);
        (void)x; // suppress compiler warning about unused variable
    } while (err != ERR && y == ypos); // keep going until EOL
}

/*
 * what happens when you ^L
 * (TODO)
 */
void chan_sub_refresh(struct chan *chan) {
    chan_title_refresh(chan);
    wclear(chan->main_win);
    int max = chan->main_lines * (chan->sub.page+1);
    if (max > chan->sub.nsubs) max = chan->sub.nsubs;
    for (int i = chan->main_lines * chan->sub.page; i < max; ++i) {
        chan_redraw_submission(chan, i);
    }
    wrefresh(chan->main_win);
    wclear(chan->status_win);
    wrefresh(chan->status_win);
}

/*
 * redraw all submissions
 *
 * (this DOES call wrefresh())
 */
void chan_sub_draw(struct chan *chan) {
    scrollok(chan->main_win, FALSE);
    chan_sub_refresh(chan);
}

/*
 * called on every keypress while on the main page
 */
#define ACTIVE chan->sub.subs[chan->sub.active]
int chan_sub_key(struct chan *chan, int ch) {
    if (ch == chan->keys.sub_down) {
        if (chan->sub.active < chan->sub.nsubs - 1) {
            ++chan->sub.active;
            // check to see if we need to scroll down a page
            if (chan->sub.active >= chan->main_lines * (chan->sub.page+1)) {
                ++chan->sub.page;
                chan_sub_refresh(chan);
            } else {
                chan_redraw_submission(chan, chan->sub.active - 1);
                chan_redraw_submission(chan, chan->sub.active);
            }
            wrefresh(chan->main_win);
        }
        return 1;
    }

    else if (ch == chan->keys.sub_up) {
        if (chan->sub.active > 0) {
            --chan->sub.active;
            if (chan->sub.active < chan->main_lines * chan->sub.page) {
                --chan->sub.page;
                chan_sub_refresh(chan);
            } else {
                chan_redraw_submission(chan, chan->sub.active + 1);
                chan_redraw_submission(chan, chan->sub.active);
            }
            wrefresh(chan->main_win);
        }
        return 1;
    }

    else if (ch == chan->keys.sub_login) {
        chan_login_init(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_open_url) {
        urlopen(ACTIVE.url);
        return 1;
    }

    else if (ch == chan->keys.sub_reload) {
        chan_sub_update(chan);
        chan_sub_draw(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_upvote) {
        if (chan->authenticated) {
            char *buf = malloc(100);
            sprintf(buf, "https://news.ycombinator.com/vote?id=%d&how=u%c&auth=%s",
                    ACTIVE.id, ACTIVE.voted ? 'n' : 'p', ACTIVE.auth);
            http(chan->curl, buf, NULL, 0);
            free(buf);

            ACTIVE.voted = 1 - ACTIVE.voted;
            ACTIVE.score += ACTIVE.voted ? 1 : -1;
            chan_redraw_submission(chan, chan->sub.active);
            wrefresh(chan->main_win);
        } else {
            wclear(chan->status_win);
            mvwaddstr(chan->status_win, 0, 0,
                    "You must be authenticated to do that.");
            wrefresh(chan->status_win);
        }
        return 1;
    }

    else if (ch == chan->keys.sub_view_comments) {
        chan->com.sub = chan->sub.subs + chan->sub.active;
        if (!chan->com.sub->coms) chan_com_update(chan);
        chan_com_draw(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_home) {
        chan->sub.mode = SUB_HOME;
        chan_sub_update(chan);
        chan_sub_draw(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_new) {
        chan->sub.mode = SUB_NEW;
        chan_sub_update(chan);
        chan_sub_draw(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_show) {
        chan->sub.mode = SUB_SHOW;
        chan_sub_update(chan);
        chan_sub_draw(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_ask) {
        chan->sub.mode = SUB_ASK;
        chan_sub_update(chan);
        chan_sub_draw(chan);
        return 1;
    }

    else if (ch == chan->keys.sub_jobs) {
        chan->sub.mode = SUB_JOBS;
        chan_sub_update(chan);
        chan_sub_draw(chan);
        return 1;
    }

    else {
        return 0;
    }
}
