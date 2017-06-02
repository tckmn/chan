#include "chan.h"

#include <locale.h>

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    struct chan *chan = chan_init(argc, argv);
    if (chan) {
        chan_main_loop(chan);
        chan_destroy(chan);
        return 0;
    } else return 1;
}
