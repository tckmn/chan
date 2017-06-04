#include "title.h"

void chan_title_refresh(struct chan *chan) {
    wclear(chan->title_win);
    wattrset(chan->title_win, A_REVERSE);
    scrollok(chan->title_win, FALSE);
    int err = OK;

    // write program name and version
    wattron(chan->title_win, A_BOLD);
    mvwaddstr(chan->title_win, 0, 0, "chan v0.0.1  ");
    wattroff(chan->title_win, A_BOLD);

    // appropriate title for the given section
    if (chan->com.sub) {
        err = waddstr(chan->title_win, chan->com.sub->title);
    } else {
        err = waddstr(chan->title_win,
                "home | new | threads | comments | show | ask | jobs | submit");
        //       13     20    26        36         47     54    60     67    (73)
        wattron(chan->title_win, A_BOLD);
        if (chan->sub.mode == SUB_HOME && COLS >= 13) {
            mvwaddstr(chan->title_win, 0, 13, "home");
        } else if (chan->sub.mode == SUB_NEW && COLS >= 20) {
            mvwaddstr(chan->title_win, 0, 20, "new");
        } else if (chan->sub.mode == SUB_SHOW && COLS >= 47) {
            mvwaddstr(chan->title_win, 0, 47, "show");
        } else if (chan->sub.mode == SUB_ASK && COLS >= 54) {
            mvwaddstr(chan->title_win, 0, 54, "ask");
        } else if (chan->sub.mode == SUB_JOBS && COLS >= 60) {
            mvwaddstr(chan->title_win, 0, 60, "jobs");
        }
        wattroff(chan->title_win, A_BOLD);
        // move back for the space-filling below
        wmove(chan->title_win, 0, 73);
    }

    // fill with spaces to create a solid bar with A_REVERSE
    while (err == OK) err = waddch(chan->title_win, ' ');

    wrefresh(chan->title_win);
}
