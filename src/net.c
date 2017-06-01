#include "net.h"

#include <stdlib.h>
#include <string.h>

struct membuf {
    char *data;
    size_t len;
};

size_t nop_callback(char *data, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}

size_t mem_callback(char *data, size_t size, size_t nmemb, void *userp) {
    size_t len = size * nmemb;
    struct membuf *buf = (struct membuf *) userp;
    buf->data = realloc(buf->data, buf->len + len);
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    return len;
}

char *do_GET(CURL *curl, const char *url) {
    struct membuf buf;
    buf.data = NULL;
    buf.len = 0;

    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_perform(curl);

    return buf.data;
}

void do_GET_nodata(CURL *curl, const char *url) {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nop_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_perform(curl);
}

void do_POST_nodata(CURL *curl, const char *url, const char *fields) {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nop_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_perform(curl);
}
