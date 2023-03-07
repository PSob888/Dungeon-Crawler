#include "stuff.h"

int main(){
    init_screen_client();
    int a=ask_for_join();
    if(a==1)
    {
        run_client();
        endwin();
    }
    else
        printf("Server full \n");
    return 0;
}

