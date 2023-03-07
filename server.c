#include "stuff.h"

pid_t server_pid;
char map[MAP_HEIGHT][MAP_WIDTH];
char map_o[MAP_HEIGHT][MAP_WIDTH];
int map_deadcoins[MAP_HEIGHT][MAP_WIDTH];
struct point_t spawnpoints[4] = {
        {2,2},
        {MAP_WIDTH-3,2},
        {2,MAP_HEIGHT-3},
        {MAP_WIDTH-3,MAP_HEIGHT-3}
}; //dopisac fajne spawny albo i nie heh
struct player_server_t players[MAX_PLAYERS];
struct connection_info_t conn;
struct point_t camppos = {
        32,15
};
struct beast_t beasts[MAX_BEASTS];
unsigned long turn;
sem_t sem_player1;
sem_t sem_player2;
sem_t sem_player3;
sem_t sem_player4;
sem_t sem_beast1;
sem_t sem_beast2;
sem_t sem_beast3;
sem_t sem_beast4;
int is_free_slot;
int is_free_beasts;
int beasts_turn_off;

int server_on()
{
    server_pid=getpid();
    is_free_slot=MAX_PLAYERS;
    conn.slots[0]=0;
    conn.slots[1]=0;
    conn.slots[2]=0;
    conn.slots[3]=0;
    is_free_beasts=MAX_BEASTS;
    conn.slots_beast[0]=0;
    conn.slots_beast[1]=0;
    conn.slots_beast[2]=0;
    conn.slots_beast[3]=0;
    int beasts_turn_off=0;
    return 0;
}

void run()
{
    turn=0;
    while(1)
    {
        print_screen_server();
        turn++;

        check_for_connection();

        //players_check_action();

        char c=getch();
        if(c=='q'||c=='Q')
            break;
        if(c=='c')
            spawn_treasure(0);
        if(c=='t')
            spawn_treasure(1);
        if(c=='T')
            spawn_treasure(2);
        if(c=='b'||c=='B')
            beast_add();

        if(conn.slots[0]==1)
            sem_post(&sem_player1);
        if(conn.slots[1]==1)
            sem_post(&sem_player2);
        if(conn.slots[2]==1)
            sem_post(&sem_player3);
        if(conn.slots[3]==1)
            sem_post(&sem_player4);
        if(conn.slots_beast[0]==1)
            sem_post(&sem_beast1);
        if(conn.slots_beast[1]==1)
            sem_post(&sem_beast2);
        if(conn.slots_beast[2]==1)
            sem_post(&sem_beast3);
        if(conn.slots_beast[3]==1)
            sem_post(&sem_beast4);
    }
    beasts_turn_off=1;
    if(conn.slots_beast[0]==1)
        sem_post(&sem_beast1);
    if(conn.slots_beast[1]==1)
        sem_post(&sem_beast2);
    if(conn.slots_beast[2]==1)
        sem_post(&sem_beast3);
    if(conn.slots_beast[3]==1)
        sem_post(&sem_beast4);
    int ajdi=0;
    for(ajdi=0;ajdi<4;ajdi++)
    {
        if(conn.slots_beast[ajdi]==1)
        {
            if(pthread_join(beasts[ajdi].beast_thread,NULL)!=0)
                if(errno==ESRCH)
                    continue;
        }

    }
    remove("fplayer_0");
    remove("fplayer_1");
    remove("fplayer_2");
    remove("fplayer_3");
    remove("player_0");
    remove("player_1");
    remove("player_2");
    remove("player_3");
}

void players_check_action()
{
    for(int i=0;i<4;i++)
    {
        if(conn.slots[i]==1)
        {
            if(players[i].player_shared.move>0&&players[i].player_shared.move<5)
                player_move(&players[i]);
            players[i].player_shared.move=0;
            map_copy_to_player(i);
        }
    }
}

void beast_add()
{
    if(is_free_beasts==0)
        return;
    is_free_beasts--;
    int x=0;
    int y=0;
    srand(time(NULL));
    while(1)
    {
        x=rand()%MAP_WIDTH;
        y=rand()%MAP_HEIGHT;
        if(map[y][x]==' ')
        {
            map[y][x]='*';
            break;
        }
    }
    int ajdi;
    for(ajdi=0;ajdi<4;ajdi++)
    {
        if(conn.slots_beast[ajdi]==0)
            break;
    }
    conn.slots_beast[ajdi]=1;
    beasts[ajdi].id=ajdi;
    beasts[ajdi].pos.x=x;
    beasts[ajdi].pos.y=y;

    pthread_create(&beasts[ajdi].beast_thread,NULL,beast_process,(void*)&beasts[ajdi]);

}

void check_for_connection()
{
    pid_t temp_pid;
    int fd=open("want_join",O_RDONLY | O_NONBLOCK);
    if(fd==-1)
        return;

    if(read(fd,&temp_pid,sizeof(pid_t))==-1)
        return;
    close(fd);
    remove("want_join");

    if(is_free_slot==0)
    {
        struct player_shared_t notallowed;
        notallowed.allow=0;
        char name[9];
        sprintf(name,"%d",temp_pid);
        if(mkfifo(name,0777)==-1)
            if(errno!=EEXIST)
                return;

        int tst=open(name,O_WRONLY);
        if(tst<0)
            return;

        if(write(tst,&notallowed,sizeof(struct player_shared_t))==-1)
        {
            close(tst);
            return;
        }
        close(tst);
    }
    else
    {
        int ajdi;
        for(ajdi=0;ajdi<4;ajdi++)
        {
            if(conn.slots[ajdi]==0)
                break;
        }
        conn.slots[ajdi]=1; // TUTAJ WYWALA SEG FAULTA ZMIENICCCCCCCCCC JAK WCHODZI 5 GRACZ

        players[ajdi].id=ajdi;
        players[ajdi].type=0;
        players[ajdi].coins=0;
        players[ajdi].coins_deposited=0;
        players[ajdi].deaths=0;
        players[ajdi].is_slowed=0;
        players[ajdi].dead=0;
        players[ajdi].pos=spawnpoints[ajdi];
        players[ajdi].spawn=spawnpoints[ajdi];
        map[spawnpoints[ajdi].y][spawnpoints[ajdi].x]=ajdi+'1';

        players[ajdi].player_shared.id=ajdi;
        players[ajdi].player_shared.pid=temp_pid;
        players[ajdi].player_shared.server_pid=server_pid;
        players[ajdi].player_shared.type=0;
        players[ajdi].player_shared.pos=players[ajdi].pos;
        players[ajdi].player_shared.coins=0;
        players[ajdi].player_shared.coins_deposited=0;
        players[ajdi].player_shared.deaths=0;
        players[ajdi].player_shared.turn=turn;
        players[ajdi].player_shared.move=0;
        players[ajdi].player_shared.close=0;
        players[ajdi].player_shared.allow=1;
        map_copy_to_player(ajdi);
        is_free_slot--;

        char name[9];
        sprintf(name,"%d",temp_pid);
        if(mkfifo(name,0777)==-1)
            if(errno!=EEXIST)
                return;

        int tst=open(name,O_WRONLY);
        if(tst<0)
            return;

        if(write(tst,&players[ajdi].player_shared,sizeof(struct player_shared_t))==-1)
        {
            close(tst);
            return;
        }
        close(tst);

        pthread_create(&players[ajdi].player_thread,NULL,player_process,(void*)&players[ajdi]);
    }


}

void* player_process(void* player)
{
    struct player_server_t *p = (struct player_server_t *)player;
    struct player_shared_t p_read;
    char from_name[10];
    sprintf(from_name,"fplayer_%d",p->id);
    if(mkfifo(from_name,0777)==-1)
        if(errno!=EEXIST)
            return NULL;
    char to_name[9];
    sprintf(to_name,"player_%d",p->id);
    if(mkfifo(to_name,0777)==-1)
        if(errno!=EEXIST)
            return NULL;


    while(1)
    {
        if(p_read.close==1)
            break;
        int trykill=kill(p->player_shared.pid,0);
        if(trykill==-1&&errno==ESRCH)
            break;

        p->player_shared.turn=turn;
        int fd2=open(to_name,O_WRONLY);
        if(fd2<0)
            continue;
        if(write(fd2,&p->player_shared,sizeof(struct player_shared_t))==-1)
            continue;
        close(fd2);

        int fd1=open(from_name,O_RDONLY);
        if(fd1<0)
            continue;
        if(read(fd1,&p_read,sizeof(struct player_shared_t))==-1)
            continue;
        close(fd1);
        p->player_shared.move=p_read.move;
        p_read.move=0;
        if(p->player_shared.move>0&&p->player_shared.move<5)
        {
            player_move(p);
            p->player_shared.move=0;
        }
        map_copy_to_player(p->id);

        if(p->id==0)
        {
            sem_wait(&sem_player1);
        }
        if(p->id==1)
        {
            sem_wait(&sem_player2);
        }
        if(p->id==2)
        {
            sem_wait(&sem_player3);
        }
        if(p->id==3)
        {
            sem_wait(&sem_player4);
        }
    }
    close_connection(p->id);

    return NULL;
}

void* beast_process(void* beast)
{
    struct beast_t *b = (struct beast_t *)beast;

    while(1)
    {
        if(beasts_turn_off>0)
            break;
        int test=beast_poscig(b);
        if(test==0)
            beast_just_move(b);
        if(b->id==0)
        {
            sem_wait(&sem_beast1);
        }
        if(b->id==1)
        {
            sem_wait(&sem_beast2);
        }
        if(b->id==2)
        {
            sem_wait(&sem_beast3);
        }
        if(b->id==3)
        {
            sem_wait(&sem_beast4);
        }
    }

    return NULL;
}

void close_connection(int id)
{
    conn.slots[id]=0;
    map[players[id].pos.y][players[id].pos.x]=map_o[players[id].pos.y][players[id].pos.x];
    pthread_join(players[id].player_thread,NULL); //--------------------TUTAJ DODAC
    is_free_slot++;
}

void map_copy_to_player(int id)
{
    int x=players[id].pos.x;
    int y=players[id].pos.y;

    for(int i=0;i<5;i++)
    {
        for(int j=0;j<5;j++)
        {
            players[id].player_shared.visible_map[i][j]=map[y-2+i][x-2+j];
        }
    }
}

void spawn_treasure(int treasure)
{
    if(treasure<0||treasure>2)
        return;
    char tre=0;
    if(treasure==0)
        tre='c';
    if(treasure==1)
        tre='t';
    if(treasure==2)
        tre='T';
    srand(time(NULL));
    while(1)
    {
        int x=rand()%MAP_WIDTH;
        int y=rand()%MAP_HEIGHT;
        if(map[y][x]==' ')
        {
            map[y][x]=tre;
            break;
        }
    }
}

void print_screen_server()
{
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_WHITE);   //WALL
    init_pair(2,COLOR_BLACK,COLOR_BLACK);   //EMPTY
    init_pair(3,COLOR_WHITE,COLOR_BLACK);   //BUSH
    init_pair(4,COLOR_YELLOW,COLOR_GREEN);  //CAMP
    init_pair(5,COLOR_BLACK,COLOR_YELLOW);  //COIN
    init_pair(6,COLOR_WHITE,COLOR_MAGENTA); //PLAYER
    init_pair(7,COLOR_WHITE,COLOR_RED);     //BEAST

    clear();
    move(0,0);
    for(int i=0;i<MAP_HEIGHT;i++)   //PRINTOWANIE
    {
        for(int j=0;j<MAP_WIDTH;j++)
        {
            if(map[i][j]=='W')
            {
                attron(COLOR_PAIR(WALL));   //WALL PRINT
                mvprintw(i,j,"%c",' ');
                attroff(COLOR_PAIR(WALL));
            }
            if(map[i][j]==' ')
            {
                attron(COLOR_PAIR(EMPTY));  //EMPTY PRINT
                mvprintw(i,j,"%c",' ');
                attroff(COLOR_PAIR(EMPTY));
            }
            if(map[i][j]=='#')
            {
                attron(COLOR_PAIR(BUSH));   //BUSH PRINT
                mvprintw(i,j,"%c",'#');
                attroff(COLOR_PAIR(BUSH));
            }
            if(map[i][j]=='A')
            {
                attron(COLOR_PAIR(CAMP));   //CAMP PRINT
                mvprintw(i,j,"%c",'A');
                attroff(COLOR_PAIR(CAMP));
            }
            if(map[i][j]=='c')
            {
                attron(COLOR_PAIR(COIN));   //COIN PRINT
                mvprintw(i,j,"%c",'c');
                attroff(COLOR_PAIR(COIN));
            }
            if(map[i][j]=='t')
            {
                attron(COLOR_PAIR(COIN));   //TREASURE PRINT
                mvprintw(i,j,"%c",'t');
                attroff(COLOR_PAIR(COIN));
            }
            if(map[i][j]=='T')
            {
                attron(COLOR_PAIR(COIN));   //TREASURE BAG PRINT
                mvprintw(i,j,"%c",'T');
                attroff(COLOR_PAIR(COIN));
            }
            if(map[i][j]=='D')
            {
                attron(COLOR_PAIR(COIN));   //DEAD BAG
                mvprintw(i,j,"%c",'D');
                attroff(COLOR_PAIR(COIN));
            }
            if(map[i][j]=='1')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i,j,"%c",'1');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(map[i][j]=='2')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i,j,"%c",'2');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(map[i][j]=='3')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i,j,"%c",'3');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(map[i][j]=='4')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i,j,"%c",'4');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(map[i][j]=='*')
            {
                attron(COLOR_PAIR(BEAST));   //BEAST PRINT
                mvprintw(i,j,"%c",'*');
                attroff(COLOR_PAIR(BEAST));
            }
        }
    }

    //RESZTA GUI

    attrset(0);
    mvprintw(1, MAP_WIDTH+2, "Server's PID: %d", server_pid);
    mvprintw(2, MAP_WIDTH+4, "Campsite X/Y: 32/15");
    mvprintw(3, MAP_WIDTH+4, "Turn number: %lu", turn);

    mvprintw(5, MAP_WIDTH+2, "Parameter:");
    mvprintw(6, MAP_WIDTH+4, "PID");
    mvprintw(7, MAP_WIDTH+4, "Type");
    mvprintw(8, MAP_WIDTH+4, "Current X/Y");
    mvprintw(9, MAP_WIDTH+4, "Deaths");
    mvprintw(11, MAP_WIDTH+4, "Coins:");
    mvprintw(12, MAP_WIDTH+8, "Carried");
    mvprintw(13, MAP_WIDTH+8, "Brought");
    mvprintw(15, MAP_WIDTH+3, "Legend:");

    attron(COLOR_PAIR(PLAYER));
    mvprintw(16, MAP_WIDTH+4, "1234");
    attrset(0);
    mvprintw(16, MAP_WIDTH+8, " - players");

    attron(COLOR_PAIR(WALL));
    mvprintw(17, MAP_WIDTH+4, "%c", ' ');
    attrset(0);
    mvprintw(17, MAP_WIDTH+5,"    - wall");

    attron(COLOR_PAIR(BUSH));
    mvprintw(18, MAP_WIDTH+4, "%c", '#');
    attrset(0);
    mvprintw(18, MAP_WIDTH+5,"    - bushes (slow down)");

    attron(COLOR_PAIR(BEAST));
    mvprintw(19, MAP_WIDTH+4, "%c", '*');
    attrset(0);
    mvprintw(19, MAP_WIDTH+5,"    - wild beast");

    attron(COLOR_PAIR(COIN));
    mvprintw(20, MAP_WIDTH+4, "%c", 'c');
    attrset(0);
    mvprintw(20, MAP_WIDTH+5,"    - one coin");

    attron(COLOR_PAIR(COIN));
    mvprintw(21, MAP_WIDTH+4, "%c", 't');
    attrset(0);
    mvprintw(21, MAP_WIDTH+5,"    - treasure (10 coins)");

    attron(COLOR_PAIR(COIN));
    mvprintw(22, MAP_WIDTH+4, "%c", 'T');
    attrset(0);
    mvprintw(22, MAP_WIDTH+5,"    - large treasure (50 coins)");

    attron(COLOR_PAIR(CAMP));
    mvprintw(23, MAP_WIDTH+4, "%c", 'A');
    attrset(0);
    mvprintw(23, MAP_WIDTH+5,"    - campsite");

    attron(COLOR_PAIR(COIN));
    mvprintw(20, MAP_WIDTH+37, "%c", 'D');
    attrset(0);
    mvprintw(20, MAP_WIDTH+38," - dropped treasure");

    for(int i=0;i<MAX_PLAYERS;i++) //DO DOPISANIA WYSWIETLANIE GRACZY Z BOKU, ALE JESZCZE NIE MAM GRACZY ZROBIONYCH
    {
        mvprintw(5, MAP_WIDTH+20+i*10, "Player%d", i+1);
        if(conn.slots[i]==1)
        {
            mvprintw(6, MAP_WIDTH+20+i*10, "%d", players[i].player_shared.pid);
            mvprintw(7, MAP_WIDTH+20+i*10, "HUMAN");
            mvprintw(8, MAP_WIDTH+20+i*10, "%d/%d", players[i].pos.x,players[i].pos.y);
            mvprintw(9, MAP_WIDTH+20+i*10, "%d", players[i].deaths);
            mvprintw(12, MAP_WIDTH+20+i*10, "%d", players[i].coins);
            mvprintw(13, MAP_WIDTH+20+i*10, "%d", players[i].coins_deposited);
        }
        else
        {
            mvprintw(6, MAP_WIDTH+20+i*10, "-----");
            mvprintw(7, MAP_WIDTH+20+i*10, "-----");
            mvprintw(8, MAP_WIDTH+20+i*10, "-----");
            mvprintw(9, MAP_WIDTH+20+i*10, "-----");
            mvprintw(12, MAP_WIDTH+20+i*10, "-----");
            mvprintw(13, MAP_WIDTH+20+i*10, "-----");
        }
    }


    mvprintw(MAP_HEIGHT+1, 1, "COMMANDS:");
    mvprintw(MAP_HEIGHT+2, 2, "B/b   - add beast somewhere");
    mvprintw(MAP_HEIGHT+3, 2, "c/t/T - add coin/treasure/large treasure somewhere");
    mvprintw(MAP_HEIGHT+4, 2, "Q/q   - quit");

    refresh();
}

void init_screen_server()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    timeout(500);
}

int load_map()
{
    FILE *f = fopen("map.txt","r");
    if(f==NULL)
        return -1;

    for(int i=0;i<MAP_HEIGHT;i++)
    {
        for(int j=0;j<MAP_WIDTH;j++)
        {
            map[i][j]=fgetc(f);
            map_o[i][j]=map[i][j];
        }
        fgetc(f);
    }
    fclose(f);

    spawn_treasures();
    return 0;
}

void spawn_treasures()
{
    srand(time(NULL));
    int spawn=0;
    int co=0;
    for(int i=0;i<MAP_HEIGHT;i++)
    {
        for(int j=0;j<MAP_WIDTH;j++)
        {
            if(map[i][j]==' ')
            {
                spawn=rand()%101;
                if(spawn>=96)
                {
                    co=rand()%10;
                    if(co<=6)
                        map[i][j]='c';
                    if(co>6&&co<=8)
                        map[i][j]='t';
                    if(co==9)
                        map[i][j]='T';
                }
            }
        }
    }
}

void basic_print()
{
    for(int i=0;i<MAP_HEIGHT;i++)
    {
        for(int j=0;j<MAP_WIDTH;j++)
        {
            printf("%c",map[i][j]);
        }
        printf("\n");
    }
}

void beast_just_move(struct beast_t *b)
{
    srand(time(NULL)); //0-lewo 1-prawo 2-gora 3-dol;
    int a=0;
    while(1)
    {
        a=rand()%4;
        if(a==0)
        {
            if(map[b->pos.y][b->pos.x-1]==' ')
            {
                map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
                b->pos.x--;
                map[b->pos.y][b->pos.x]='*';
                break;
            }
        }
        if(a==1)
        {
            if(map[b->pos.y][b->pos.x+1]==' ')
            {
                map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
                b->pos.x++;
                map[b->pos.y][b->pos.x]='*';
                break;
            }
        }
        if(a==2)
        {
            if(map[b->pos.y-1][b->pos.x]==' ')
            {
                map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
                b->pos.y--;
                map[b->pos.y][b->pos.x]='*';
                break;
            }
        }
        if(a==3)
        {
            if(map[b->pos.y+1][b->pos.x]==' ')
            {
                map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
                b->pos.y++;
                map[b->pos.y][b->pos.x]='*';
                break;
            }
        }
    }
}

int beast_poscig(struct beast_t *b)
{
    int want_to_poscig=0;
    //szukanie gracza w gore mapy
    for(int i=1;i<MAP_HEIGHT;i++)
    {
        if(map[b->pos.y-i][b->pos.x]=='1')
        {
            want_to_poscig=1;
            break;
        }
        if(map[b->pos.y-i][b->pos.x]=='2')
        {
            want_to_poscig=1;
            break;
        }
        if(map[b->pos.y-i][b->pos.x]=='3')
        {
            want_to_poscig=1;
            break;
        }
        if(map[b->pos.y-i][b->pos.x]=='4')
        {
            want_to_poscig=1;
            break;
        }
        if(map[b->pos.y-i][b->pos.x]=='W')
        {
            break;
        }
        if(map[b->pos.y-i][b->pos.x]=='#')
        {
            break;
        }
    }

    //szukanie gracza w dol mapy
    for(int i=1;i<MAP_HEIGHT;i++)
    {
        if(map[b->pos.y+i][b->pos.x]=='1')
        {
            want_to_poscig=2;
            break;
        }
        if(map[b->pos.y+i][b->pos.x]=='2')
        {
            want_to_poscig=2;
            break;
        }
        if(map[b->pos.y+i][b->pos.x]=='3')
        {
            want_to_poscig=2;
            break;
        }
        if(map[b->pos.y+i][b->pos.x]=='4')
        {
            want_to_poscig=2;
            break;
        }
        if(map[b->pos.y+i][b->pos.x]=='W')
        {
            break;
        }
        if(map[b->pos.y+i][b->pos.x]=='#')
        {
            break;
        }
    }

    //szukanie gracza w lewo mapy
    for(int i=1;i<MAP_WIDTH;i++)
    {
        if(map[b->pos.y][b->pos.x-i]=='1')
        {
            want_to_poscig=3;
            break;
        }
        if(map[b->pos.y][b->pos.x-i]=='2')
        {
            want_to_poscig=3;
            break;
        }
        if(map[b->pos.y][b->pos.x-i]=='3')
        {
            want_to_poscig=3;
            break;
        }
        if(map[b->pos.y][b->pos.x-i]=='4')
        {
            want_to_poscig=3;
            break;
        }
        if(map[b->pos.y][b->pos.x-i]=='W')
        {
            break;
        }
        if(map[b->pos.y][b->pos.x-i]=='#')
        {
            break;
        }
    }

    for(int i=1;i<MAP_WIDTH;i++)
    {
        if(map[b->pos.y][b->pos.x+i]=='1')
        {
            want_to_poscig=4;
            break;
        }
        if(map[b->pos.y][b->pos.x+i]=='2')
        {
            want_to_poscig=4;
            break;
        }
        if(map[b->pos.y][b->pos.x+i]=='3')
        {
            want_to_poscig=4;
            break;
        }
        if(map[b->pos.y][b->pos.x+i]=='4')
        {
            want_to_poscig=4;
            break;
        }
        if(map[b->pos.y][b->pos.x+i]=='W')
        {
            break;
        }
        if(map[b->pos.y][b->pos.x+i]=='#')
        {
            break;
        }
    }

    //obsluga ruchu jak jest want to poscig
    if(want_to_poscig==1)
    {
        if(map[b->pos.y-1][b->pos.x]=='1')
        {
            players[0].deaths++;
            players[0].player_shared.deaths++;
            map[players[0].pos.y][players[0].pos.x]='D';
            map_deadcoins[players[0].pos.y][players[0].pos.x]+=players[0].coins;
            players[0].coins=0;
            players[0].player_shared.coins=0;
            players[0].pos.x=players[0].spawn.x;
            players[0].pos.y=players[0].spawn.x;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.x;
            map[players[0].pos.y][players[0].pos.x]='1';
            map_copy_to_player(0);
        }
        else if(map[b->pos.y-1][b->pos.x]=='2')
        {
            players[1].deaths++;
            players[1].player_shared.deaths++;
            map[players[1].pos.y][players[1].pos.x]='D';
            map_deadcoins[players[1].pos.y][players[1].pos.x]+=players[1].coins;
            players[1].coins=0;
            players[1].player_shared.coins=0;
            players[1].pos.x=players[1].spawn.x;
            players[1].pos.y=players[1].spawn.x;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.x;
            map[players[1].pos.y][players[1].pos.x]='2';
            map_copy_to_player(1);
        }
        else if(map[b->pos.y-1][b->pos.x]=='3')
        {
            players[2].deaths++;
            players[2].player_shared.deaths++;
            map[players[2].pos.y][players[2].pos.x]='D';
            map_deadcoins[players[2].pos.y][players[2].pos.x]+=players[2].coins;
            players[2].coins=0;
            players[2].player_shared.coins=0;
            players[2].pos.x=players[2].spawn.x;
            players[2].pos.y=players[2].spawn.x;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.x;
            map[players[2].pos.y][players[2].pos.x]='3';
            map_copy_to_player(2);
        }
        else if(map[b->pos.y-1][b->pos.x]=='4')
        {
            players[3].deaths++;
            players[3].player_shared.deaths++;
            map[players[3].pos.y][players[3].pos.x]='D';
            map_deadcoins[players[3].pos.y][players[3].pos.x]+=players[3].coins;
            players[3].coins=0;
            players[3].player_shared.coins=0;
            players[3].pos.x=players[3].spawn.x;
            players[3].pos.y=players[3].spawn.x;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.x;
            map[players[3].pos.y][players[3].pos.x]='4';
            map_copy_to_player(3);
        }
        else
        {
            map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
            map[b->pos.y-1][b->pos.x]='*';
            b->pos.y=b->pos.y-1;
        }
    }

    if(want_to_poscig==2)
    {
        if(map[b->pos.y+1][b->pos.x]=='1')
        {
            players[0].deaths++;
            players[0].player_shared.deaths++;
            map[players[0].pos.y][players[0].pos.x]='D';
            map_deadcoins[players[0].pos.y][players[0].pos.x]+=players[0].coins;
            players[0].coins=0;
            players[0].player_shared.coins=0;
            players[0].pos.x=players[0].spawn.x;
            players[0].pos.y=players[0].spawn.x;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.x;
            map[players[0].pos.y][players[0].pos.x]='1';
            map_copy_to_player(0);
        }
        else if(map[b->pos.y+1][b->pos.x]=='2')
        {
            players[1].deaths++;
            players[1].player_shared.deaths++;
            map[players[1].pos.y][players[1].pos.x]='D';
            map_deadcoins[players[1].pos.y][players[1].pos.x]+=players[1].coins;
            players[1].coins=0;
            players[1].player_shared.coins=0;
            players[1].pos.x=players[1].spawn.x;
            players[1].pos.y=players[1].spawn.x;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.x;
            map[players[1].pos.y][players[1].pos.x]='2';
            map_copy_to_player(1);
        }
        else if(map[b->pos.y+1][b->pos.x]=='3')
        {
            players[2].deaths++;
            players[2].player_shared.deaths++;
            map[players[2].pos.y][players[2].pos.x]='D';
            map_deadcoins[players[2].pos.y][players[2].pos.x]+=players[2].coins;
            players[2].coins=0;
            players[2].player_shared.coins=0;
            players[2].pos.x=players[2].spawn.x;
            players[2].pos.y=players[2].spawn.x;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.x;
            map[players[2].pos.y][players[2].pos.x]='3';
            map_copy_to_player(2);
        }
        else if(map[b->pos.y+1][b->pos.x]=='4')
        {
            players[3].deaths++;
            players[3].player_shared.deaths++;
            map[players[3].pos.y][players[3].pos.x]='D';
            map_deadcoins[players[3].pos.y][players[3].pos.x]+=players[3].coins;
            players[3].coins=0;
            players[3].player_shared.coins=0;
            players[3].pos.x=players[3].spawn.x;
            players[3].pos.y=players[3].spawn.x;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.x;
            map[players[3].pos.y][players[3].pos.x]='4';
            map_copy_to_player(3);
        }
        else
        {
            map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
            map[b->pos.y+1][b->pos.x]='*';
            b->pos.y=b->pos.y+1;
        }
    }

    if(want_to_poscig==3)
    {
        if(map[b->pos.y][b->pos.x-1]=='1')
        {
            players[0].deaths++;
            players[0].player_shared.deaths++;
            map[players[0].pos.y][players[0].pos.x]='D';
            map_deadcoins[players[0].pos.y][players[0].pos.x]+=players[0].coins;
            players[0].coins=0;
            players[0].player_shared.coins=0;
            players[0].pos.x=players[0].spawn.x;
            players[0].pos.y=players[0].spawn.x;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.x;
            map[players[0].pos.y][players[0].pos.x]='1';
            map_copy_to_player(0);
        }
        else if(map[b->pos.y][b->pos.x-1]=='2')
        {
            players[1].deaths++;
            players[1].player_shared.deaths++;
            map[players[1].pos.y][players[1].pos.x]='D';
            map_deadcoins[players[1].pos.y][players[1].pos.x]+=players[1].coins;
            players[1].coins=0;
            players[1].player_shared.coins=0;
            players[1].pos.x=players[1].spawn.x;
            players[1].pos.y=players[1].spawn.x;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.x;
            map[players[1].pos.y][players[1].pos.x]='2';
            map_copy_to_player(1);
        }
        else if(map[b->pos.y][b->pos.x-1]=='3')
        {
            players[2].deaths++;
            players[2].player_shared.deaths++;
            map[players[2].pos.y][players[2].pos.x]='D';
            map_deadcoins[players[2].pos.y][players[2].pos.x]+=players[2].coins;
            players[2].coins=0;
            players[2].player_shared.coins=0;
            players[2].pos.x=players[2].spawn.x;
            players[2].pos.y=players[2].spawn.x;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.x;
            map[players[2].pos.y][players[2].pos.x]='3';
            map_copy_to_player(2);
        }
        else if(map[b->pos.y][b->pos.x-1]=='4')
        {
            players[3].deaths++;
            players[3].player_shared.deaths++;
            map[players[3].pos.y][players[3].pos.x]='D';
            map_deadcoins[players[3].pos.y][players[3].pos.x]+=players[3].coins;
            players[3].coins=0;
            players[3].player_shared.coins=0;
            players[3].pos.x=players[3].spawn.x;
            players[3].pos.y=players[3].spawn.x;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.x;
            map[players[3].pos.y][players[3].pos.x]='4';
            map_copy_to_player(3);
        }
        else
        {
            map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
            map[b->pos.y][b->pos.x-1]='*';
            b->pos.x=b->pos.x-1;
        }
    }

    if(want_to_poscig==4)
    {
        if(map[b->pos.y][b->pos.x+1]=='1')
        {
            players[0].deaths++;
            players[0].player_shared.deaths++;
            map[players[0].pos.y][players[0].pos.x]='D';
            map_deadcoins[players[0].pos.y][players[0].pos.x]+=players[0].coins;
            players[0].coins=0;
            players[0].player_shared.coins=0;
            players[0].pos.x=players[0].spawn.x;
            players[0].pos.y=players[0].spawn.x;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.x;
            map[players[0].pos.y][players[0].pos.x]='1';
            map_copy_to_player(0);
        }
        else if(map[b->pos.y][b->pos.x+1]=='2')
        {
            players[1].deaths++;
            players[1].player_shared.deaths++;
            map[players[1].pos.y][players[1].pos.x]='D';
            map_deadcoins[players[1].pos.y][players[1].pos.x]+=players[1].coins;
            players[1].coins=0;
            players[1].player_shared.coins=0;
            players[1].pos.x=players[1].spawn.x;
            players[1].pos.y=players[1].spawn.x;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.x;
            map[players[1].pos.y][players[1].pos.x]='2';
            map_copy_to_player(1);
        }
        else if(map[b->pos.y][b->pos.x+1]=='3')
        {
            players[2].deaths++;
            players[2].player_shared.deaths++;
            map[players[2].pos.y][players[2].pos.x]='D';
            map_deadcoins[players[2].pos.y][players[2].pos.x]+=players[2].coins;
            players[2].coins=0;
            players[2].player_shared.coins=0;
            players[2].pos.x=players[2].spawn.x;
            players[2].pos.y=players[2].spawn.x;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.x;
            map[players[2].pos.y][players[2].pos.x]='3';
            map_copy_to_player(2);
        }
        else if(map[b->pos.y][b->pos.x+1]=='4')
        {
            players[3].deaths++;
            players[3].player_shared.deaths++;
            map[players[3].pos.y][players[3].pos.x]='D';
            map_deadcoins[players[3].pos.y][players[3].pos.x]+=players[3].coins;
            players[3].coins=0;
            players[3].player_shared.coins=0;
            players[3].pos.x=players[3].spawn.x;
            players[3].pos.y=players[3].spawn.x;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.x;
            map[players[3].pos.y][players[3].pos.x]='4';
            map_copy_to_player(3);
        }
        else
        {
            map[b->pos.y][b->pos.x]=map_o[b->pos.y][b->pos.x];
            map[b->pos.y][b->pos.x+1]='*';
            b->pos.x=b->pos.x+1;
        }
    }
    if(want_to_poscig==0)
        return 0;
    return 1;
}

void player_move(struct player_server_t *player) //1-gora, 2-dol, 3-lewo, 4-prawo
{
    if(player->is_slowed==1)
    {
        player->is_slowed=0;
        return;
    }
    //gora
    if(player->player_shared.move==2) //SCIANA, KRZAK, COINY, TRERZYR, BIG TRERZYR, UPUSZCZONY LOOT CAMP, GRACZ, BESTIA
    {
        if(map[player->pos.y+1][player->pos.x]=='W')
            return;
        if(map[player->pos.y+1][player->pos.x]==' ')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='#')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            player->is_slowed=1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='c')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            player->coins++;
            player->player_shared.coins++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='t')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            player->coins+=10;
            player->player_shared.coins+=10;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='T')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            player->coins+=50;
            player->player_shared.coins+=50;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='D')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            player->coins+=map_deadcoins[player->pos.y][player->pos.x];
            player->player_shared.coins+=map_deadcoins[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x]=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='A')
        {
            player->pos.y=player->pos.y+1;
            player->player_shared.pos.y=player->player_shared.pos.y+1;
            player->coins_deposited=player->coins;
            player->player_shared.coins_deposited=player->coins;
            player->coins=0;
            player->player_shared.coins=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y-1][player->pos.x]=map_o[player->pos.y-1][player->pos.x];
            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='1')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y+1][player->pos.x]+=player->coins+players[0].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[0].coins=0;
            players[0].player_shared.coins=0;
            players[0].player_shared.deaths++;
            players[0].pos.y=player->spawn.y;
            players[0].pos.x=player->spawn.x;
            players[0].player_shared.pos.y=player->spawn.y;
            players[0].player_shared.pos.x=player->spawn.x;
            players[0].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[0].pos.y][players[0].pos.x]=players[0].id+'1';
            map[y][x]='D';


            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='2')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y+1][player->pos.x]+=player->coins+players[1].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[1].coins=0;
            players[1].player_shared.coins=0;
            players[1].player_shared.deaths++;
            players[1].pos.y=player->spawn.y;
            players[1].pos.x=player->spawn.x;
            players[1].player_shared.pos.y=player->spawn.y;
            players[1].player_shared.pos.x=player->spawn.x;
            players[1].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[1].pos.y][players[1].pos.x]=players[1].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='3')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y+1][player->pos.x]+=player->coins+players[2].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[2].coins=0;
            players[2].player_shared.coins=0;
            players[2].player_shared.deaths++;
            players[2].pos.y=player->spawn.y;
            players[2].pos.x=player->spawn.x;
            players[2].player_shared.pos.y=player->spawn.y;
            players[2].player_shared.pos.x=player->spawn.x;
            players[2].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[2].pos.y][players[2].pos.x]=players[2].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='4')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y+1][player->pos.x]+=player->coins+players[3].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[3].coins=0;
            players[3].player_shared.coins=0;
            players[3].player_shared.deaths++;
            players[3].pos.y=player->spawn.y;
            players[3].pos.x=player->spawn.x;
            players[3].player_shared.pos.y=player->spawn.y;
            players[3].player_shared.pos.x=player->spawn.x;
            players[3].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[3].pos.y][players[3].pos.x]=players[3].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y+1][player->pos.x]=='*')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y+1][player->pos.x]+=player->coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[y][x]='D';

            return;
        }
    }
    //dol
    if(player->player_shared.move==1) //SCIANA, KRZAK, COINY, TRERZYR, BIG TRERZYR, UPUSZCZONY LOOT CAMP, GRACZ, BESTIA
    {
        if(map[player->pos.y-1][player->pos.x]=='W')
            return;
        if(map[player->pos.y-1][player->pos.x]==' ')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='#')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            player->is_slowed=1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='c')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            player->coins++;
            player->player_shared.coins++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='t')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            player->coins+=10;
            player->player_shared.coins+=10;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='T')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            player->coins+=50;
            player->player_shared.coins+=50;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='D')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            player->coins+=map_deadcoins[player->pos.y][player->pos.x];
            player->player_shared.coins+=map_deadcoins[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x]=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='A')
        {
            player->pos.y=player->pos.y-1;
            player->player_shared.pos.y=player->player_shared.pos.y-1;
            player->coins_deposited=player->coins;
            player->player_shared.coins_deposited=player->coins;
            player->coins=0;
            player->player_shared.coins=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y+1][player->pos.x]=map_o[player->pos.y+1][player->pos.x];
            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='1')//TUTAJ
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y-1][player->pos.x]+=player->coins+players[0].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[0].player_shared.coins=0;
            players[0].player_shared.deaths++;
            players[0].coins=0;
            players[0].pos.y=players[0].spawn.y;
            players[0].pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.y;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[0].pos.y][players[0].pos.x]=players[0].id+'1';
            map[y][x]='D';


            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='2')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y-1][player->pos.x]+=player->coins+players[1].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[1].player_shared.coins=0;
            players[1].player_shared.deaths++;
            players[1].coins=0;
            players[1].pos.y=players[1].spawn.y;
            players[1].pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.y;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[1].pos.y][players[1].pos.x]=players[1].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='3')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y-1][player->pos.x]+=player->coins+players[2].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[2].player_shared.coins=0;
            players[2].player_shared.deaths++;
            players[2].coins=0;
            players[2].pos.y=players[2].spawn.y;
            players[2].pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.y;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[2].pos.y][players[2].pos.x]=players[2].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='4')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y-1][player->pos.x]+=player->coins+players[3].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[3].player_shared.coins=0;
            players[3].player_shared.deaths++;
            players[3].coins=0;
            players[3].pos.y=players[3].spawn.y;
            players[3].pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.y;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[3].pos.y][players[3].pos.x]=players[3].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y-1][player->pos.x]=='*')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y-1][player->pos.x]+=player->coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[y][x]='D';

            return;
        }
    }

    //lewo
    if(player->player_shared.move==3) //SCIANA, KRZAK, COINY, TRERZYR, BIG TRERZYR, UPUSZCZONY LOOT CAMP, GRACZ, BESTIA
    {
        if(map[player->pos.y][player->pos.x-1]=='W')
            return;
        if(map[player->pos.y][player->pos.x-1]==' ')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='#')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            player->is_slowed=1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='c')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            player->coins++;
            player->player_shared.coins++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='t')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            player->coins+=10;
            player->player_shared.coins+=10;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='T')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            player->coins+=50;
            player->player_shared.coins+=50;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='D')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            player->coins+=map_deadcoins[player->pos.y][player->pos.x];
            player->player_shared.coins+=map_deadcoins[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x]=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='A')
        {
            player->pos.x=player->pos.x-1;
            player->player_shared.pos.x=player->player_shared.pos.x-1;
            player->coins_deposited=player->coins;
            player->player_shared.coins_deposited=player->coins;
            player->player_shared.coins=0;
            player->coins=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x+1]=map_o[player->pos.y][player->pos.x+1];
            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='1')//TUTAJ
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x-1]+=player->coins+players[0].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[0].player_shared.coins=0;
            players[0].player_shared.deaths++;
            players[0].coins=0;
            players[0].pos.y=players[0].spawn.y;
            players[0].pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.y;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[0].pos.y][players[0].pos.x]=players[0].id+'1';
            map[y][x]='D';


            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='2')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x-1]+=player->coins+players[1].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[1].player_shared.coins=0;
            players[1].player_shared.deaths++;
            players[1].coins=0;
            players[1].pos.y=players[1].spawn.y;
            players[1].pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.y;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[1].pos.y][players[1].pos.x]=players[1].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='3')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x-1]+=player->coins+players[2].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[2].player_shared.coins=0;
            players[2].player_shared.deaths++;
            players[2].coins=0;
            players[2].pos.y=players[2].spawn.y;
            players[2].pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.y;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[2].pos.y][players[2].pos.x]=players[2].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='4')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x-1]+=player->coins+players[3].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[3].player_shared.coins=0;
            players[3].player_shared.deaths++;
            players[3].coins=0;
            players[3].pos.y=players[3].spawn.y;
            players[3].pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.y;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[3].pos.y][players[3].pos.x]=players[3].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y][player->pos.x-1]=='*')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x-1]+=player->coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[y][x]='D';

            return;
        }
    }

    //prawo
    if(player->player_shared.move==4) //SCIANA, KRZAK, COINY, TRERZYR, BIG TRERZYR, UPUSZCZONY LOOT CAMP, GRACZ, BESTIA
    {
        if(map[player->pos.y][player->pos.x+1]=='W')
            return;
        if(map[player->pos.y][player->pos.x+1]==' ')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='#')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            player->is_slowed=1;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='c')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            player->coins++;
            player->player_shared.coins++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='t')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            player->coins+=10;
            player->player_shared.coins+=10;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='T')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            player->coins+=50;
            player->player_shared.coins+=50;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='D')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            player->coins+=map_deadcoins[player->pos.y][player->pos.x];
            player->player_shared.coins+=map_deadcoins[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x]=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='A')
        {
            player->pos.x=player->pos.x+1;
            player->player_shared.pos.x=player->player_shared.pos.x+1;
            player->coins_deposited=player->coins;
            player->player_shared.coins_deposited=player->coins;
            player->player_shared.coins=0;
            player->coins=0;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[player->pos.y][player->pos.x-1]=map_o[player->pos.y][player->pos.x-1];
            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='1')//TUTAJ
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x+1]+=player->coins+players[0].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[0].player_shared.coins=0;
            players[0].player_shared.deaths++;
            players[0].coins=0;
            players[0].pos.y=players[0].spawn.y;
            players[0].pos.x=players[0].spawn.x;
            players[0].player_shared.pos.y=players[0].spawn.y;
            players[0].player_shared.pos.x=players[0].spawn.x;
            players[0].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[0].pos.y][players[0].pos.x]=players[0].id+'1';
            map[y][x]='D';


            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='2')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x+1]+=player->coins+players[1].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[1].player_shared.coins=0;
            players[1].player_shared.deaths++;
            players[1].coins=0;
            players[1].pos.y=players[1].spawn.y;
            players[1].pos.x=players[1].spawn.x;
            players[1].player_shared.pos.y=players[1].spawn.y;
            players[1].player_shared.pos.x=players[1].spawn.x;
            players[1].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[1].pos.y][players[1].pos.x]=players[1].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='3')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x+1]+=player->coins+players[2].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[2].player_shared.coins=0;
            players[2].player_shared.deaths++;
            players[2].coins=0;
            players[2].pos.y=players[2].spawn.y;
            players[2].pos.x=players[2].spawn.x;
            players[2].player_shared.pos.y=players[2].spawn.y;
            players[2].player_shared.pos.x=players[2].spawn.x;
            players[2].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[2].pos.y][players[2].pos.x]=players[2].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='4')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x+1]+=player->coins+players[3].coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            players[3].player_shared.coins=0;
            players[3].player_shared.deaths++;
            players[3].coins=0;
            players[3].pos.y=players[3].spawn.y;
            players[3].pos.x=players[3].spawn.x;
            players[3].player_shared.pos.y=players[3].spawn.y;
            players[3].player_shared.pos.x=players[3].spawn.x;
            players[3].deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[players[3].pos.y][players[3].pos.x]=players[3].id+'1';
            map[y][x]='D';

            return;
        }
        if(map[player->pos.y][player->pos.x+1]=='*')
        {
            int x=player->pos.x;
            int y=player->pos.y+1;
            map[player->pos.y][player->pos.x]=map_o[player->pos.y][player->pos.x];
            map_deadcoins[player->pos.y][player->pos.x+1]+=player->coins;
            player->pos.y=player->spawn.y;
            player->pos.x=player->spawn.x;
            player->player_shared.pos.y=player->spawn.y;
            player->player_shared.pos.x=player->spawn.x;
            player->coins=0;
            player->deaths++;
            player->player_shared.coins=0;
            player->player_shared.deaths++;
            map[player->pos.y][player->pos.x]=player->id+'1';
            map[y][x]='D';

            return;
        }
    }
}
