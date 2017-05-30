#include <ncurses.h>
#include <curl/curl.h>

int main(int argc, char **argv) {
    initscr();

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "http://httpbin.org/ip");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, printw);
    curl_easy_perform(curl);
    curl_global_cleanup();

    refresh();

    getch();

    endwin();
    return 0;
}
