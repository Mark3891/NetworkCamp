#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <ctype.h>

#define BUF_SIZE 256

typedef struct
{
    int frequency;
    char name[BUF_SIZE];
} list;

void error_handling(char *message);
void print_Colored(WINDOW *win, const char *name, const char *message, int frequency);
void update_display(WINDOW *win, list *send_data, int count, const char *message);

int main(int argc, char *argv[])
{
    int sd;

    struct sockaddr_in serv_adr;
    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));

    initscr();
    WINDOW *search_win = newwin(2, COLS, 0, 0);
    WINDOW *result_win = newwin(10, COLS, 2, 0);
    char message[BUF_SIZE] = "";
    int msg_len = 0;

    while (1)
    {   
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        wclear(search_win);
        wattron(search_win, COLOR_PAIR(1));
        mvwprintw(search_win, 0, 0, "Search Word:");
        wattroff(search_win, COLOR_PAIR(1));

        wattron(search_win, COLOR_PAIR(2));
        mvwprintw(search_win, 0, 13, "%s", message);
        wattroff(search_win, COLOR_PAIR(2));
        wrefresh(search_win);

        int ch = getch();

        if (ch == 27)
        {
            break;
        }
        else if (ch == 127 || ch == KEY_BACKSPACE) 
        {   
           if (msg_len > 0)
            {
                msg_len--;
                message[msg_len] = '\0';
            }
        }
        else if (isalpha(ch) || ch == ' ') 
        {
            if (msg_len < BUF_SIZE - 1)
            {
                message[msg_len++] = ch;
                message[msg_len] = '\0';
            }
        }
        else
        {
            continue;
        }

        if(msg_len==0){
            message[0]='\0';
            continue;
        }

        write(sd, message, BUF_SIZE);
        int recv_count;
        read(sd,&recv_count,sizeof(int));
        
        list *recv_data = (list *)malloc(recv_count*sizeof(list));
        int total_received = 0;
        while (total_received < recv_count*sizeof(list))
        {
            int recv_len = read(sd, ((char *)recv_data) + total_received,  recv_count*sizeof(list)- total_received);
            if (recv_len == -1)
            {
                error_handling("read() error");
            }
            total_received += recv_len;
        }

   
       

      
        init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
        update_display(result_win, recv_data, recv_count, message);
       
    }

    close(sd);
    endwin(); // End ncurses mode
    return 0;
}

void error_handling(char *message)
{
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

void print_Colored(WINDOW *win, const char *name, const char *message, int frequency)
{
    const char *match;
    int index = 0;
    int len = strlen(message);
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);

    while ((match = strstr(name + index, message)) != NULL)
    {
        wprintw(win, "%.*s", (int)(match - (name + index)), name + index);
        wattron(win, COLOR_PAIR(3));
        wprintw(win, "%.*s", len, match);
        wattroff(win, COLOR_PAIR(3));
        index = (match - name) + len;
    }
    wprintw(win, "%s, %d\n", name + index, frequency);
}

void update_display(WINDOW *win, list *send_data, int count, const char *message)
{
    wclear(win);
    for (int i = 0; i < count; i++)
    {
        wprintw(win, "%d: ", i + 1);
        print_Colored(win, send_data[i].name, message, send_data[i].frequency);
    }
    wrefresh(win);
}
