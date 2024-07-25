#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUF_SIZE 100
#define FILE_SIZE 4096
typedef struct
{
    unsigned int filesize;
    unsigned char filename[BUF_SIZE];
} file_pkt;

typedef struct
{
    file_pkt file_list[BUF_SIZE];
    unsigned int file_number;
} list_pkt;

void error_handling(char *message);
void print_listFile(list_pkt *list_file);

int main(int argc, char *argv[])
{

    int sock;
    char message[BUF_SIZE];
    int str_len, recv_len, recv_cnt;
    struct sockaddr_in serv_adr;

    FILE *fp;
    char buf[FILE_SIZE];
    int read_cnt;

    if (argc != 3)
    {
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

    connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr));

    puts("Connected...........");

    file_pkt *recv_file;
    recv_file = (file_pkt *)malloc(sizeof(file_pkt));
    list_pkt *list_file;
    list_file = (list_pkt *)malloc(sizeof(list_pkt));
    memset(recv_file, 0, sizeof(file_pkt));
    memset(list_file, 0, sizeof(list_pkt));

    int total_received = 0;
    while (total_received < sizeof(list_pkt))
    {
        int recv_len = read(sock, ((char *)list_file) + total_received, sizeof(list_pkt) - total_received);
        if (recv_len == -1)
        {
            error_handling("read() error");
        }
        total_received += recv_len;
    }

    if (total_received != sizeof(list_pkt))
    {
        error_handling("Failed to read complete list_pkt struct");
    }

    print_listFile(list_file);

    while (1)
    {

        fputs("Input Index(Q to quit, 0 to print_listFile): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
            break;
        else if (!strcmp(message, "0\n"))
        {
            print_listFile(list_file);
            continue;
        }else{
            write(sock, message, strlen(message));
        // index 보내기

        fp = fopen((char *)list_file->file_list[atoi(message) - 1].filename, "wb");

        int total_read = 0;

        while ((read_cnt = read(sock, buf, FILE_SIZE)) != 0)
        {
            if (read_cnt == -1)
                error_handling("read() error");

            total_read += read_cnt;

            fwrite((void *)buf, 1, read_cnt, fp);

            if (total_read >= list_file->file_list[atoi(message) - 1].filesize)
                break;
        }

        fclose(fp);
        puts("Received file data");

        }

       
    }
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void print_listFile(list_pkt *list_file)
{
    int i = 0;
    while (strcmp(list_file->file_list[i].filename, "") != 0)
    {

        printf("%d 번째-> filename : %s, filesize: %d\n", i + 1, list_file->file_list[i].filename, list_file->file_list[i].filesize);
        i++;
    }
}
