#include "submissions.h"
#include "login.h"
#include "auth.h"

#include <stdlib.h>
#include <string.h>

void chan_login_init(struct chan *chan) {
    curs_set(1);

    chan->username = malloc(1);
    chan->username[0] = '\0';

    wclear(chan->status_win);
    waddstr(chan->status_win, "Username: ");
    wrefresh(chan->status_win);
}

int addkey(struct chan *chan, char **str, int ch) {
    if (' ' <= ch && ch <= '~') {
        if (!chan->password) {
            waddch(chan->status_win, ch);
            wrefresh(chan->status_win);
        }

        int oldlen = strlen(*str);
        *str = realloc(*str, oldlen + 2);
        (*str)[oldlen] = ch;
        (*str)[oldlen+1] = '\0';

        return 1;
    } else if (ch == '\x7f' && (*str)[0]) {
        // backspace
        if (!chan->password) {
            waddstr(chan->status_win, "\b \b");
            wrefresh(chan->status_win);
        }

        (*str)[strlen(*str) - 1] = '\0';

        return 1;
    } else if (ch == '\n') {
        if (chan->password) {
            int success = auth(chan->curl, chan->username, chan->password);

            free(chan->username);
            chan->username = NULL;
            free(chan->password);
            chan->password = NULL;

            if (success) {
                chan->authenticated = 1;
                chan_update_submissions(chan);
                chan_draw_submissions(chan);
            }

            wclear(chan->status_win);
            waddstr(chan->status_win,
                    success ? "Authenticated" : "Invalid credentials");
            wrefresh(chan->status_win);

            curs_set(0);
        } else {
            chan->password = malloc(1);
            chan->password[0] = '\0';

            wclear(chan->status_win);
            waddstr(chan->status_win, "Password: ");
            wrefresh(chan->status_win);
        }

        return 1;
    } else return 0;
}

int chan_login_key(struct chan *chan, int ch) {
    return addkey(chan, chan->password ? &chan->password : &chan->username, ch);
}
