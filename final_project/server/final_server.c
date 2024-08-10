#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <time.h>

#define BUF_SIZE 100
#define EPOLL_SIZE 50

void error_handling(char *message);
void initialize_array(int **a);
void *timer_thread(void *arg);

typedef struct
{
    int index;
    int player_id; // clnt_sock
    int team;      // red: 1, blue: 2
    int x;
    int y;
} player;

typedef struct
{
    int player_number;
    int play_time;
    int room_width;
    int room_height;
    int tile_num;
    player init_p;
} board_info;

typedef struct
{
    int clnt_sock;
    int player_number;
} ThreadArgs;

typedef struct
{
    int **board;
    player *players;
    int time_remaining;
} game_info;

board_info *board;
game_info *game;
int connected_clients = 0;
int expected_clients;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
ThreadArgs *client_args;
player *p;
 int epfd, event_cnt;
 int serv_sock, clnt_sock;

int main(int argc, char *argv[])
{
    board = (board_info *)malloc(sizeof(board_info));
    memset(board, 0, sizeof(board_info));

    int port;
    if (argc != 11)
    {
        printf("Usage : %s -n <player> -s <width,height> -b <tile_num> -t <play_time> -p <port>\n", argv[0]);
        exit(1);
    }
    int a[5] = {0};
    for (int i = 1; i < 11; i += 2)
    {
        if (strcmp(argv[i], "-n") == 0)
        {
            board->player_number = atoi(argv[i + 1]);
            expected_clients = board->player_number;
            a[0] = 1;
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            board->room_height = atoi(argv[i + 1]);
            board->room_width = atoi(argv[i + 1]);
            a[1] = 1;
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            board->tile_num = atoi(argv[i + 1]);
            a[2] = 1;
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            board->play_time = atoi(argv[i + 1]);
            a[3] = 1;
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[i + 1]);
            a[4] = 1;
        }
    }
    if (a[0] != 1)
    {
        printf("-n 제대로 입력해주세요\n");
        exit(1);
    }
    else if (a[1] != 1)
    {
        printf("-s 제대로 입력해주세요\n");
        exit(1);
    }
    else if (a[2] != 1)
    {
        printf("-b 제대로 입력해주세요\n");
        exit(1);
    }
    else if (a[3] != 1)
    {
        printf("-t 제대로 입력해주세요\n");
        exit(1);
    }
    else if (a[4] != 1)
    {
        printf("-p 제대로 입력해주세요\n");
        exit(1);
    }


    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    int str_len, i;
    char buf[BUF_SIZE];

    struct epoll_event *ep_events;
    struct epoll_event event;
   
    srand(time(NULL));

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port);
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    client_args = (ThreadArgs *)malloc(sizeof(ThreadArgs) * expected_clients);
    p = (player *)malloc(sizeof(player) * expected_clients);

    // game_info 동적할당
    game = (game_info *)malloc(sizeof(game_info));
    game->board = (int **)malloc(sizeof(int *) * board->room_height);
    for (int i = 0; i < board->room_height; i++)
    {
        game->board[i] = (int *)malloc(sizeof(int) * board->room_width);
    }

    // 랜덤 게임판 만들기
    initialize_array(game->board);

    for (int i = 0; i < board->room_height; i++)
    {
        for (int j = 0; j < board->room_width; j++)
        {
            printf("%d ", game->board[i][j]);
        }
        printf("\n");
    }

    game->players = (player *)malloc(expected_clients * sizeof(player));

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1); // 무한 대기
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }

        for (i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("connected client: %d \n", clnt_sock);

                client_args[connected_clients].clnt_sock = clnt_sock;
                client_args[connected_clients].player_number = board->player_number;
                connected_clients++;

                if (connected_clients == expected_clients)
                {

                    // 여기에서 각 player들의 struct구성해줘야 한다.
                    for (int j = 0; j < expected_clients; j++)
                    {

                        p[j].index = j;
                        p[j].player_id = client_args[j].clnt_sock;
                        p[j].team = (j % 2 == 0) ? 1 : 2; // 랜덤 팀 배정
                        p[j].x = (j % 2 == 0) ? 0 : board->room_width - 1;
                        p[j].y = (j % 2 == 0) ? 0 : board->room_height - 1; // 배열로 놓을 것이라 1을 뺌

                        // 클라이언트마다 board_info 생성
                        board_info client_board = *board;
                        client_board.init_p = p[j];

                        write(client_args[j].clnt_sock, &client_board, sizeof(board_info));
                    }
                    game->players = p;
                    game->time_remaining = board->play_time;
                    pthread_t t_thread;
                    pthread_create(&t_thread, NULL, timer_thread, &game->time_remaining);
                    for (int i = 0; i < expected_clients; i++)
                    {
                        // 2차배열 보내기
                        for (int j = 0; j < board->room_height; j++)
                        {
                            write(p[i].player_id, game->board[j], sizeof(int) * board->room_width);
                        }
                        // game player 보내기
                        write(p[i].player_id, game->players, sizeof(player) * expected_clients);
                    }
                }
            }
            else
            {
                int index; //해당 유저 찾기
                int ch; // 명령어 숫자로 바꾸기
                int sock = ep_events[i].data.fd;

                str_len = read(sock, buf, BUF_SIZE);
                if (str_len == 0)
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client: %d \n", ep_events[i].data.fd);
                    connected_clients--;
                    continue;
                }
                printf("clnt sock: %d, buf: %s\n", sock, buf);
                
                for(int i=0;i<expected_clients;i++){
                    if(p[i].player_id==sock){
                        index=i;
                    }
                }

                if(strcmp(buf,"UP")==0){
                    ch=1;
                }else if(strcmp(buf,"DOWN")==0){
                    ch=2;
                }else if(strcmp(buf,"LEFT")==0){
                    ch=3;
                }else if(strcmp(buf,"RIGHT")==0){
                    ch=4;
                }else if(strcmp(buf,"SPACE")==0){
                    ch=5;
                }
                switch (ch)
                {
                case 1:
                    if (game->players[index].y > 0)
                    {
                        game->players[index].y--;
                       
                    }

                    break;
                case 2:
                    if (game->players[index].y < board->room_height - 1)
                    {
                         game->players[index].y++;
                     
                    }

                    break;
                case 3:
                    if (game->players[index].x > 0)
                    {
                          game->players[index].x--;
           
                    }

                    break;
                case 4:
                    if (game->players[index].x < board->room_width - 1)
                    {
                          game->players[index].x++;
               
                    }

                    break;
            
                case 5:  // SPACEBAR key
                    if (game->board[game->players[index].y][game->players[index].x] != 0)
                    {
                        game->board[game->players[index].y][game->players[index].x] = game->players[index].team;
               
                    }
                    break;
                default:
                    break;
                }
                

                for (int i = 0; i < expected_clients; i++)
                {
                    for (int j = 0; j < board->room_height; j++)
                    {
                        write(p[i].player_id, game->board[j], sizeof(int) * board->room_width);
                    }
                    write(p[i].player_id, game->players, sizeof(player) * expected_clients);
                }
            }
        }
    }
    close(serv_sock);
    close(epfd);
    free(client_args);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void initialize_array(int **a)
{
    int i, j;

    // 배열을 0으로 초기화
    for (i = 0; i < board->room_height; i++)
    {
        for (j = 0; j < board->room_width; j++)
        {
            a[i][j] = 0;
        }
    }

    // 랜덤으로 10개의 2를 배열에 부여
    for (i = 0; i < board->tile_num / 2; i++)
    {
        int row, col;
        do
        {
            row = rand() % board->room_width;
            col = rand() % board->room_height;
        } while (a[row][col] != 0); // 이미 할당된 경우 다시 선택
        a[row][col] = 2;
    }

    // 랜덤으로 10개의 1을 배열에 부여
    for (i = 0; i < board->tile_num / 2; i++)
    {
        int row, col;
        do
        {
            row = rand() % board->room_width;
            col = rand() % board->room_height;
        } while (a[row][col] != 0); // 이미 할당된 경우 다시 선택
        a[row][col] = 1;
    }
}

void *timer_thread(void *arg)
{
    int *time_remaining = (int *)arg;
    while (*time_remaining > 0)
    {
        sleep(1);
        pthread_mutex_lock(&mutex);
        (*time_remaining)--;
        printf("time: %d\n", game->time_remaining);
        pthread_mutex_unlock(&mutex);
    }
    if(*time_remaining==0){
     
                for (int i = 0; i < expected_clients; i++)
                {
                    for (int j = 0; j < board->room_height; j++)
                    {
                        write(p[i].player_id, game->board[j], sizeof(int) * board->room_width);
                    }
                    write(p[i].player_id, game->players, sizeof(player) * expected_clients);
                }
        
    }
    

    return NULL;
}