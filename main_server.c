#include "stuff.h"

int main() {
    load_map();
    //basic_print();
    init_screen_server();
    server_on();
    run();
    endwin();
    return 0;
}
