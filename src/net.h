#ifndef __NET_H__
#define __NET_H__

#include <curl/curl.h>

char *do_GET(CURL *curl, const char *url);

#endif
