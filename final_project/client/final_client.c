#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <pthread.h>

#define BUF_SIZE 100

typedef struct {
    int index;
    int player_id; // clnt_sock
    int team;      // red: 1, blue: 2
    int x;
    int y;
} player;

typedef struct {
    int player_number;
    int play_time;
    int room_width;
    int room_height;
    int tile_num;
    player init_p;
} board_info;

typedef struct {
    int **board;
    player *players;
    int time_remaining;
} game_info;

board_info board;
game_info *game;
int sock;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int run_threads = 1; // Flag to control thread execution

void error_handling(char *message);

void *write_thread(void *arg);
void *read_thread(void *arg);
void draw_borders(int grid_width, int grid_height);
void fill_cells(int grid_width, int grid_height, int **a);
void update_screen(int grid_width, int grid_height, int **a, player *p, int time_remaining);
void fill_player(int grid_width, int grid_height, player p[board.player_number]);
void *timer_thread(void *arg);

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_adr;
    int ready;

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error!");

    read(sock, &board, sizeof(board_info));
    printf("Player Number: %d\n", board.player_number);
    printf("Play Time: %d\n", board.play_time);
    printf("Room Width: %d\n", board.room_width);
    printf("Room Height: %d\n", board.room_height);
    printf("Tile Number: %d\n", board.tile_num);
    printf("Player Index: %d\n", board.init_p.index);
    printf("Player ClntSock: %d\n", board.init_p.player_id);
    printf("Player Team: %d\n", board.init_p.team);
    printf("Player Initial Position: (%d, %d)\n", board.init_p.x, board.init_p.y);

    // 게임 인포 받기
    game = (game_info *)malloc(sizeof(game_info));
    game->board = (int **)malloc(sizeof(int *) * board.room_height);
    for (int i = 0; i < board.room_height; i++) {
        game->board[i] = (int *)malloc(sizeof(int) * board.room_width);
        read(sock, game->board[i], sizeof(int) * board.room_width);
    }

    game->players = (player *)malloc(sizeof(player) * board.player_number);
    read(sock, game->players, sizeof(player) * board.player_number);

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE); // 방향키 입력을 받기 위해
    start_color();      

    // 판 색상
    init_pair(1, COLOR_RED, COLOR_RED);   // 기본 빨간색
    init_pair(2, COLOR_BLUE, COLOR_BLUE); // 기본 파란색

    //Player 색상
    init_color(11, 1000, 500, 500); // 핑크색
    init_color(22, 500, 500, 1000); // 하늘색

    init_pair(3, COLOR_BLACK, 11); // 핑크색
    init_pair(4, COLOR_BLACK, 22); // 하늘색

    int term_width, term_height;
    int grid_width, grid_height;
    // 터미널 크기 가져오기
    getmaxyx(stdscr, term_height, term_width);

    // 격자 크기 계산 (2:1 비율, 최대 크기를 터미널 크기에 맞춤)
    grid_width = term_width / (board.room_width * 2);
    grid_height = term_height / board.room_height;

    // 타이머 관련 변수 및 스레드 생성
    game->time_remaining = board.play_time;
    pthread_t t_thread;
    pthread_create(&t_thread, NULL, timer_thread, &game->time_remaining);

    pthread_t w_thread, r_thread;
    pthread_create(&r_thread, NULL, read_thread, &game->time_remaining);
    pthread_create(&w_thread, NULL, write_thread, NULL);

    pthread_join(w_thread, NULL);
    pthread_join(r_thread, NULL);



    endwin();
    printf("adsfsadfsdaf");
    close(sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void draw_borders(int grid_width, int grid_height) {
    int i, j;

    // 가로 구분선
    for (i = 0; i <= board.room_width; i++) {
        for (j = 0; j <= board.room_width * grid_width * 2; j++) {
            mvaddch(i * grid_height, j, '-');
        }
    }

    // 세로 구분선
    for (i = 0; i <= board.room_width; i++) {
        for (j = 0; j <= board.room_width * grid_height; j++) {
            mvaddch(j, i * grid_width * 2, '|');
        }
    }

    // 교차점에 '+' 표시
    for (i = 0; i <= board.room_width; i++) {
        for (j = 0; j <= board.room_width; j++) {
            mvaddch(j * grid_height, i * grid_width * 2, '+');
        }
    }
}

void fill_cells(int grid_width, int grid_height, int **a) {
    int i, j;
    for (i = 0; i < board.room_height; i++) {
        for (j = 0; j < board.room_width; j++) {
            int start_x = j * grid_width * 2 + 2;
            int start_y = i * grid_height + 1;
            int end_x = start_x + grid_width * 2 - 4;
            int end_y = start_y + grid_height - 2;

            // 색상 선택
            int color;
            if (a[i][j] == 1) {
                color = 1; // 빨간색
            } else if (a[i][j] == 2) {
                color = 2; // 파란색
            } else {
                continue; // 0인 경우 색칠하지 않음
            }

            // 색상 설정
            attron(COLOR_PAIR(color));
            // 셀 내부 색칠
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
                    mvaddch(y, x, ' ');
                }
            }
            attroff(COLOR_PAIR(color));
        }
    }
}

void fill_player(int grid_width, int grid_height, player p[board.player_number]) {
    for (int i = 0; i < board.player_number; i++) {
        int start_x = p[i].x * grid_width * 2 + 1;
        int start_y = p[i].y * grid_height + 1;
        int end_x = start_x + grid_width * 2 - 2;
        int end_y = start_y + grid_height - 2;

        int color;
        if (p[i].team == 1) {
            color = 3; // 핑크색
        } else if (p[i].team == 2) {
            color = 4; // 하늘색
        } else {
            return; // 유효하지 않은 팀
        }

        // 색상 설정
        attron(COLOR_PAIR(color));
        // 플레이어의 위치에 색칠 (가장자리)
        for (int y = start_y; y <= end_y; y++) {
            mvaddch(y, start_x, ' ');
            mvaddch(y, end_x, ' ');
        }
        for (int x = start_x; x <= end_x; x++) {
            mvaddch(start_y, x, ' ');
            mvaddch(end_y, x, ' ');
        }
        if (p[i].player_id == board.init_p.player_id)
            mvprintw(start_y, start_x + 1, "me");
        attroff(COLOR_PAIR(color));
    }
}

void update_screen(int grid_width, int grid_height, int **a, player *p, int time_remaining) {
    clear();
    draw_borders(grid_width, grid_height);
    fill_cells(grid_width, grid_height, a);
    fill_player(grid_width, grid_height, p);
    int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
    mvprintw(term_height - 1, term_width - 20, "Time Remaining: %d", time_remaining); // 타이머를 화면의 맨 아래 오른쪽에 표시
    refresh();
}

void *timer_thread(void *arg) {
    int *time_remaining = (int *)arg;
    while (run_threads && *time_remaining > 0) {
        sleep(1);
        pthread_mutex_lock(&mutex);
        (*time_remaining)--;
        pthread_mutex_unlock(&mutex);

        int term_width, term_height;
        getmaxyx(stdscr, term_height, term_width);
        int grid_width = term_width / (board.room_width * 2);
        int grid_height = term_height / board.room_height;

        pthread_mutex_lock(&mutex);
        update_screen(grid_width, grid_height, game->board, game->players, *time_remaining);
        pthread_mutex_unlock(&mutex);
    }

    // 타이머 종료 시 모든 스레드를 종료
    run_threads = 0;

    return NULL;
}

void *write_thread(void *arg) {
    char buf[BUF_SIZE];
    int ch;
    while (run_threads) // 'q' key to exit
    {
        ch = getch();
        switch (ch) {
            case KEY_UP:
                if (game->players[board.init_p.index].y > 0)
                    strcpy(buf, "UP");
                break;
            case KEY_DOWN:
                if (game->players[board.init_p.index].y < board.room_height - 1)
                    strcpy(buf, "DOWN");
                break;
            case KEY_LEFT:
                if (game->players[board.init_p.index].x > 0)
                    strcpy(buf, "LEFT");
                break;
            case KEY_RIGHT:
                if (game->players[board.init_p.index].x < board.room_width - 1)
                    strcpy(buf, "RIGHT");
                break;
            case ' ':  // SPACEBAR key
                if (game->board[game->players[board.init_p.index].y][game->players[board.init_p.index].x] != 0)
                    strcpy(buf, "SPACE");
                break;
            default:
                break;
        }
        write(sock, buf, BUF_SIZE);
    }
    run_threads = 0; // Stop read thread
    return NULL;
}

void *read_thread(void *arg) {
    int *time_remaining = (int *)arg;
    while (run_threads) {
        for (int i = 0; i < board.room_height; i++) {
            if (read(sock, game->board[i], sizeof(int) * board.room_width) <= 0) {
                perror("read board error");
                return NULL;
            }
        }
        if (read(sock, game->players, sizeof(player) * board.player_number) <= 0) {
            perror("read players error");
            return NULL;
        }

        int term_width, term_height;
        getmaxyx(stdscr, term_height, term_width);
        int grid_width = term_width / (board.room_width * 2);
        int grid_height = term_height / board.room_height;

        pthread_mutex_lock(&mutex);
        update_screen(grid_width, grid_height, game->board, game->players, *time_remaining);
        pthread_mutex_unlock(&mutex);

    }
    return NULL;
}
