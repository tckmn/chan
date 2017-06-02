#ifndef __AUTH_H__
#define __AUTH_H__

#include "chan.h"

#include <curl/curl.h>

int auth(CURL *curl, char *username, char *password);

#endif
