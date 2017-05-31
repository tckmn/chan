#include "chan.h"

int main(int argc, char **argv) {
    struct chan *chan = chan_init();
    chan_main_loop(chan);
    chan_destroy(chan);
    return 0;
}
