
#ifndef GIERA_STUFF_H
#define GIERA_STUFF_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <ncurses.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

//do kolorkow
#define WALL 1
#define EMPTY 2
#define BUSH 3
#define CAMP 4
#define COIN 5
#define PLAYER 6
#define BEAST 7
//do kolorkow

#define MAP_WIDTH 62
#define MAP_HEIGHT 27
#define MAX_PLAYERS 4
#define MAX_BEASTS 4

struct point_t {
    int x;
    int y;
};

struct player_shared_t {
    int id;
    pid_t pid;
    pid_t server_pid;
    int allow;
    int type;
    struct point_t pos;
    char visible_map[5][5];

    int coins;
    int coins_deposited;
    int deaths;
    unsigned long turn;

    int move;

    int close;
};

struct player_server_t {
    int id;
    int type; //0-human   1-bot   2-beast
    struct player_shared_t player_shared;
    struct point_t pos;
    struct point_t spawn;
    int coins;
    int coins_deposited;
    int deaths;
    int is_slowed;
    int dead;
    pthread_t player_thread;
};

struct beast_t {
    struct point_t pos;
    int id;
    pthread_t beast_thread;
};

struct connection_info_t {
    int slots[MAX_PLAYERS];
    int slots_beast[MAX_BEASTS];
    sem_t connection;
};

//funkcje serwer
int load_map();
void spawn_treasures();
void spawn_treasure(int treasure);
void init_screen_server();
void print_screen_server();
void run();
int server_on();
void map_copy_to_player(int id);
void check_for_connection();
void* player_process(void* player);
void beast_add();
void* beast_process(void* beast);
int beast_poscig(struct beast_t *b);
void beast_just_move(struct beast_t *b);
void close_connection(int id);//---------TUTAJ DODAC
void players_check_action();
void player_move(struct player_server_t *player);
void basic_print(); //do testu mapy w IDE


//funkcje klient
void init_screen_client();
void print_screen_client();
int ask_for_join(); //0-nie join, 1-join, 2-full
void send_data();
void recieve_data();
void run_client();


#endif //GIERA_STUFF_H
