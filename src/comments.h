#ifndef __COMMENTS_H__
#define __COMMENTS_H__

#include "chan.h"

void chan_destroy_comments(struct chan *chan);
void chan_update_comments(struct chan *chan);
void chan_draw_comments(struct chan *chan);
void chan_comments_key(struct chan *chan, int ch);

#endif
