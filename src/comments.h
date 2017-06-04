#ifndef __COMMENTS_H__
#define __COMMENTS_H__

#include "chan.h"

void chan_com_destroy(struct chan *chan);
void chan_com_update(struct chan *chan);
void chan_com_draw(struct chan *chan);
int chan_com_key(struct chan *chan, int ch);

#endif
