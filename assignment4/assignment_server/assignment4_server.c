#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <ctype.h>

#define BUF_SIZE 256
#define MAX_CLNT 256

void error_handling(char *message);
void *handle_clnt(void *arg);
int compare(const void *a, const void *b);
int countLines(FILE *file);

typedef struct
{
    int frequency;
    char name[BUF_SIZE];
} list;

typedef struct
{
    int clnt_sock;
    int line_count;
    list *dataList;
} ThreadArgs;

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

void print_send_data(list *send_data, int count)
{
    printf("Sending data:\n");
    for (int i = 0; i < count; i++)
    {
        printf("Name: %s, Frequency: %d\n", send_data[i].name, send_data[i].frequency);
    }
}

int main(int argc, char *argv[])
{
    int serv_sd, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;

    if (argc != 3)
    {
        printf("Usage: %s <port> <filename>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);

    serv_sd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    listen(serv_sd, 5);

    FILE *file = fopen(argv[2], "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        return 1;
    }

    int lineCount = countLines(file);
    printf("lineCount: %d\n", lineCount);

    list *dataList = (list *)malloc(lineCount * sizeof(list));
    if (dataList == NULL)
    {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    char line[BUF_SIZE];
    int i = 0;
    while (fgets(line, BUF_SIZE, file) != NULL)
    {
        sscanf(line, "%[^\t\n0-9] %d", dataList[i].name, &dataList[i].frequency);
        int len = strlen(dataList[i].name);
        while (len > 0 && dataList[i].name[len - 1] == ' ')
        {
            dataList[i].name[len - 1] = '\0';
            len--;
        }
        printf("%s,\t %d\n", dataList[i].name, dataList[i].frequency);
        i++;
    }
    fclose(file);

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sd, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        args->clnt_sock = clnt_sock;
        args->line_count = lineCount;
        args->dataList = dataList;

        pthread_create(&t_id, NULL, handle_clnt, (void *)args);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sd);
    return 0;
}

void *handle_clnt(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int clnt_sock = args->clnt_sock;
    int line_count = args->line_count;
    list *dataList = args->dataList;

    while (1)
    {
        list *search_data = (list *)malloc(line_count * sizeof(list));
        memset(search_data, 0, line_count * sizeof(list));
        list *send_data = (list *)malloc(10 * sizeof(list));
        memset(send_data, 0, 10 * sizeof(list));
        char msg[BUF_SIZE];
        int str_len = read(clnt_sock, msg, BUF_SIZE);
        if (str_len == 0)
        {
            close(clnt_sock);
            printf("Closed client: %d \n", clnt_sock);
            break;
        }
        msg[str_len] = '\0';
        printf("Message: %s\n\n", msg);

    
        int j = 0;
        for (int i = 0; i < line_count; i++)
        {
            if (strstr(dataList[i].name, msg))
            {
                strcpy(search_data[j].name, dataList[i].name);
                search_data[j].frequency = dataList[i].frequency;
                j++;
            }
        }

    
        qsort(search_data, j, sizeof(list), compare);

        int send_count = j < 10 ? j : 10;

        for (int i = 0; i < send_count; i++)
        {
            strcpy(send_data[i].name, search_data[i].name);
            send_data[i].frequency = search_data[i].frequency;
        }

        print_send_data(send_data, send_count);

        write(clnt_sock,&send_count, sizeof(int));

   
        write(clnt_sock, send_data, send_count * sizeof(list));
    }

    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);

    return NULL;
}

int compare(const void *a, const void *b)
{
    list *pa = (list *)a;
    list *pb = (list *)b;

    if (pa->frequency < pb->frequency)
        return 1;
    else if (pa->frequency > pb->frequency)
        return -1;
    else
        return 0;
}

int countLines(FILE *file)
{
    int lines = 0;
    char a;
    while ((a = fgetc(file)) != EOF)
    {
        if (a == '\n')
        {
            lines++;
        }
    }
    fseek(file, 0, SEEK_SET);
    return lines;
}
