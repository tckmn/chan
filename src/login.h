#ifndef __LOGIN_H__
#define __LOGIN_H__

#include "chan.h"

void chan_login_init(struct chan *chan);
int chan_login_key(struct chan *chan, int ch);

#endif
