#ifndef __SUBMISSIONS_H__
#define __SUBMISSIONS_H__

#include "chan.h"

void chan_destroy_submissions(struct chan *chan);
void chan_update_submissions(struct chan *chan);
void chan_draw_submissions(struct chan *chan);
int chan_submissions_key(struct chan *chan, int ch);

#endif
