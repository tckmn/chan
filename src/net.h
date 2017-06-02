#ifndef __NET_H__
#define __NET_H__

#include <curl/curl.h>

char *http(CURL *curl, const char *url, const char *postfields, int data);

#endif
