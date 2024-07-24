#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
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

int filesize(char *filename);
void error_handling(char *message);
void print_listFile(list_pkt *list_file);
void find_list(list_pkt *list_file);

int main(int argc, char *argv[])
{

    DIR *dp;
    struct dirent *dirp;
    file_pkt *send_file;
    list_pkt *list_file;

    FILE *fp;
    char buf[FILE_SIZE];
    int read_cnt;

    int serv_sock;
    int clnt_sock;
    char message[BUF_SIZE];
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_adr_sz;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // convert unsigned integer from host byte to network byte order
    serv_addr.sin_port = htons(atoi(argv[1]));     // convert unsigned short integer from host byte to network byte order

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");
    clnt_adr_sz = sizeof(clnt_addr);

    send_file = (file_pkt *)malloc(sizeof(file_pkt));
    memset(send_file, 0, sizeof(file_pkt));
    list_file = (list_pkt *)malloc(sizeof(list_pkt));
    memset(list_file, 0, sizeof(list_pkt));

    dp = opendir("./");
    int i = 0;
    while ((dirp = readdir(dp)) != NULL)
    {
        if (dirp->d_type == DT_REG)
        {
            strcpy(send_file->filename, dirp->d_name);
            send_file->filesize = filesize(dirp->d_name);
            list_file->file_list[i] = *send_file;
            i++;
           
        }
    }
    list_file->file_number = i;
    printf("%d\n",list_file->file_number);
    closedir(dp);

    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_adr_sz);
    if (clnt_sock == -1)
        error_handling("accept() error");

    write(clnt_sock, list_file, sizeof(list_pkt));

    while (1)
    {
        // print_listFile(list_file);
        read(clnt_sock, message, sizeof(message));
        int file_index = atoi(message) - 1;
        printf("index : %d, filename: %s\n", file_index + 1, list_file->file_list[file_index].filename);

        fp = fopen((char *)list_file->file_list[file_index].filename, "rb");
        while (1)
        {
            read_cnt = fread((void *)buf, 1, FILE_SIZE, fp);

            if (read_cnt > 0)
            {
                write(clnt_sock, buf, read_cnt);
            }
            if (read_cnt < FILE_SIZE)
            {
                if (ferror(fp))
                {
                    error_handling("fread() error");
                }
                break;
            }
        }
        fclose(fp);
    }

    close(clnt_sock);
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int filesize(char *filename)
{

    struct stat file_info;
    int sz_file;

    if (0 > stat(filename, &file_info))
    {
        return -1; // file이 없거나 에러
    }
    return file_info.st_size;
}

void print_listFile(list_pkt *list_file)
{

    for (int i = 0; i < list_file->file_number; i++)
    {

        printf("%d 번째-> filename : %s, filesize: %d\n", i + 1, list_file->file_list[i].filename, list_file->file_list[i].filesize);
    }
}