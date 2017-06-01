#include "auth.h"
#include "net.h"

#include <stdlib.h>
#include <string.h>

void auth(CURL *curl, char *username, char *password) {
    // URL encode POST data
    char *esc_username = curl_easy_escape(curl, username, 0),
         *esc_password = curl_easy_escape(curl, password, 0);
    char *buf = malloc(strlen(esc_username) + strlen(esc_password) + 10);
    sprintf(buf, "acct=%s&pw=%s", esc_username, esc_password);
    curl_free(esc_username);
    curl_free(esc_password);

    // perform the request
    do_POST_nodata(curl, "https://news.ycombinator.com/login", buf);

    free(buf);
}
