#include "stuff.h"

#define EXPECTED_HEIGHT 128
#define EXPECTED_WIDTH 128

struct player_shared_t mystruct;
struct point_t camppos = {
        32,15
};
pid_t mypid;

void run_client()
{
    while(1)
    {
        int trykill=kill(mystruct.server_pid,0);
        if(trykill==-1&&errno==ESRCH)
        {
            mvprintw(0,0,"SERWER CLOSED");
            refresh();
            sleep(2);
            return;
        }
        print_screen_client();
        recieve_data();
        int c=0;
        c=getch();
        if(c=='q'||c=='Q')
        {
            mystruct.close=1;
            send_data();
            break;
        }
        if(c==KEY_UP)
        {
            mystruct.move=1;
        }
        if(c==KEY_DOWN)
        {
            mystruct.move=2;
        }
        if(c==KEY_LEFT)
        {
            mystruct.move=3;
        }
        if(c==KEY_RIGHT)
        {
            mystruct.move=4;
        }
        send_data();
        c=0;
    }
}

void recieve_data()
{
    char to_name[10];
    sprintf(to_name,"player_%d",mystruct.id);
    if(mkfifo(to_name,0777)==-1)
        if(errno!=EEXIST)
            return;

    int fd=open(to_name,O_RDONLY);
    if(fd<0)
        return;
    if(read(fd,&mystruct,sizeof(struct player_shared_t))==-1)
        return;
    close(fd);
}

void send_data()
{
    char from_name[10];
    sprintf(from_name,"fplayer_%d",mystruct.id);
    if(mkfifo(from_name,0777)==-1)
        if(errno!=EEXIST)
            return;

    int fd=open(from_name,O_WRONLY);
    if(fd<0)
        return;
    if(write(fd,&mystruct,sizeof(struct player_shared_t))==-1)
        return;
    close(fd);
    mystruct.move=0;
}

int ask_for_join()
{
    mypid=getpid();
    if(mkfifo("want_join",0777)==-1)
        if(errno!=EEXIST)
            return 3;
    char name[9];
    sprintf(name,"%d",mypid);
    if(mkfifo(name,0777)==-1)
        if(errno!=EEXIST)
            return 9;

    int fd=open("want_join",O_WRONLY);
    if(fd==-1)
        return 4;
    if(write(fd,&mypid,sizeof(pid_t))==-1)
        return 5;

    close(fd);

    fd=open(name,O_RDONLY);
    if(fd==-1)
        return 6;
    if(read(fd,&mystruct,sizeof(struct player_shared_t))==-1)
        return 7;
    close(fd);
    remove(name);

    if(mystruct.allow==0)
        return 2;

    return 1;
}

void print_screen_client()
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
    for(int i=0;i<5;i++)   //PRINTOWANIE
    {
        for(int j=0;j<5;j++)
        {
            if(mystruct.visible_map[i][j]=='W')
            {
                attron(COLOR_PAIR(WALL));   //WALL PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",' ');
                attroff(COLOR_PAIR(WALL));
            }
            if(mystruct.visible_map[i][j]==' ')
            {
                attron(COLOR_PAIR(EMPTY));  //EMPTY PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",' ');
                attroff(COLOR_PAIR(EMPTY));
            }
            if(mystruct.visible_map[i][j]=='#')
            {
                attron(COLOR_PAIR(BUSH));   //BUSH PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'#');
                attroff(COLOR_PAIR(BUSH));
            }
            if(mystruct.visible_map[i][j]=='A')
            {
                attron(COLOR_PAIR(CAMP));   //CAMP PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'A');
                attroff(COLOR_PAIR(CAMP));
            }
            if(mystruct.visible_map[i][j]=='c')
            {
                attron(COLOR_PAIR(COIN));   //COIN PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'c');
                attroff(COLOR_PAIR(COIN));
            }
            if(mystruct.visible_map[i][j]=='t')
            {
                attron(COLOR_PAIR(COIN));   //TREASURE PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'t');
                attroff(COLOR_PAIR(COIN));
            }
            if(mystruct.visible_map[i][j]=='T')
            {
                attron(COLOR_PAIR(COIN));   //TREASURE BAG PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'T');
                attroff(COLOR_PAIR(COIN));
            }
            if(mystruct.visible_map[i][j]=='D')
            {
                attron(COLOR_PAIR(COIN));   //DEAD BAG
                mvprintw(i,j,"%c",'D');
                attroff(COLOR_PAIR(COIN));
            }
            if(mystruct.visible_map[i][j]=='1')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'1');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(mystruct.visible_map[i][j]=='2')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'2');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(mystruct.visible_map[i][j]=='3')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'3');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(mystruct.visible_map[i][j]=='4')
            {
                attron(COLOR_PAIR(PLAYER));   //PLAYER PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'4');
                attroff(COLOR_PAIR(PLAYER));
            }
            if(mystruct.visible_map[i][j]=='*')
            {
                attron(COLOR_PAIR(BEAST));   //BEAST PRINT
                mvprintw(i+mystruct.pos.y-2,j+mystruct.pos.x-2,"%c",'*');
                attroff(COLOR_PAIR(BEAST));
            }
        }
    }

    attrset(0);
    mvprintw(1, EXPECTED_WIDTH+2, "Server's PID: %d", mystruct.server_pid);
    mvprintw(2, EXPECTED_WIDTH+4, "Campsite X/Y: %d/%d",camppos.x,camppos.y);
    mvprintw(3, EXPECTED_WIDTH+4, "Round number: %lu", mystruct.turn);

    mvprintw(5, EXPECTED_WIDTH+2, "Player: ");
    mvprintw(6, EXPECTED_WIDTH+2, "Number:     %d",mystruct.id+1);
    mvprintw(7, EXPECTED_WIDTH+2, "Type:       HUMAN");
    mvprintw(8, EXPECTED_WIDTH+2, "Curr X/Y:   %d/%d",mystruct.pos.x,mystruct.pos.y);
    mvprintw(9, EXPECTED_WIDTH+2, "Deaths:     %d",mystruct.deaths);

    mvprintw(11, EXPECTED_WIDTH+2, "Coins found %d",mystruct.coins);
    mvprintw(12, EXPECTED_WIDTH+2, "Coins brought %d",mystruct.coins_deposited);

    mvprintw(15, EXPECTED_WIDTH+3, "Legend:");

    attron(COLOR_PAIR(PLAYER));
    mvprintw(16, EXPECTED_WIDTH+4, "1234");
    attrset(0);
    mvprintw(16, EXPECTED_WIDTH+8, " - players");

    attron(COLOR_PAIR(WALL));
    mvprintw(17, EXPECTED_WIDTH+4, "%c", ' ');
    attrset(0);
    mvprintw(17, EXPECTED_WIDTH+5,"    - wall");

    attron(COLOR_PAIR(BUSH));
    mvprintw(18, EXPECTED_WIDTH+4, "%c", '#');
    attrset(0);
    mvprintw(18, EXPECTED_WIDTH+5,"    - bushes (slow down)");

    attron(COLOR_PAIR(BEAST));
    mvprintw(19, EXPECTED_WIDTH+4, "%c", '*');
    attrset(0);
    mvprintw(19, EXPECTED_WIDTH+5,"    - wild beast");

    attron(COLOR_PAIR(COIN));
    mvprintw(20, EXPECTED_WIDTH+4, "%c", 'c');
    attrset(0);
    mvprintw(20, EXPECTED_WIDTH+5,"    - one coin");

    attron(COLOR_PAIR(COIN));
    mvprintw(21, EXPECTED_WIDTH+4, "%c", 't');
    attrset(0);
    mvprintw(21, EXPECTED_WIDTH+5,"    - treasure (10 coins)");

    attron(COLOR_PAIR(COIN));
    mvprintw(22, EXPECTED_WIDTH+4, "%c", 'T');
    attrset(0);
    mvprintw(22, EXPECTED_WIDTH+5,"    - large treasure (50 coins)");

    attron(COLOR_PAIR(CAMP));
    mvprintw(23, EXPECTED_WIDTH+4, "%c", 'A');
    attrset(0);
    mvprintw(23, EXPECTED_WIDTH+5,"    - campsite");

    attron(COLOR_PAIR(COIN));
    mvprintw(20, EXPECTED_WIDTH+37, "%c", 'D');
    attrset(0);
    mvprintw(20, EXPECTED_WIDTH+38," - dropped treasure");

    mvprintw(25, EXPECTED_WIDTH+2, "Q/q   - quit");

    refresh();
}

void init_screen_client()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    timeout(500);
}

