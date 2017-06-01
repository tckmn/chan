#ifndef __NET_H__
#define __NET_H__

#include <curl/curl.h>

char *do_GET(CURL *curl, const char *url);
void do_GET_nodata(CURL *curl, const char *url);
void do_POST_nodata(CURL *curl, const char *url, const char *fields);

#endif
