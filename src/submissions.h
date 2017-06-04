#ifndef __SUBMISSIONS_H__
#define __SUBMISSIONS_H__

#include "chan.h"

void chan_sub_destroy(struct chan *chan);
void chan_sub_update(struct chan *chan);
void chan_sub_draw(struct chan *chan);
int chan_sub_key(struct chan *chan, int ch);

#endif
