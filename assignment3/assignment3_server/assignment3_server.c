// 서버 코드
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

#define BUF_SIZE 256
#define FILE_SIZE 4096

#define CD 1 // 디렉토리 변경
#define LS 2 // 디렉토리 안 출력
#define DL 3 // 다운로드
#define UP 4 // 업로드

char buf[BUF_SIZE];

typedef struct
{
    unsigned int filesize;
    unsigned char filename[BUF_SIZE];
} file_pkt;

typedef struct
{
    unsigned char filename[BUF_SIZE];
} folder_pkt;

typedef struct
{
    file_pkt *file_list;
    folder_pkt *folder_list;
    int file_count;
    int folder_count;
} list_pkt;

typedef struct
{
    int clnt_sock;
    char ab_path[BUF_SIZE];
    list_pkt list;
} ThreadArgs;

void error_handling(char *message);
char *get_absolute_path(char *path);
int list_files_folders(char *path, list_pkt *list);
int get_file_size(char *file_path);
void *handle_clnt(void *arg);
void recv_getlist(int clnt_sock, ThreadArgs *arg);

int clnt_cnt = 0;
int clnt_socks[BUF_SIZE];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    int serv_sock;
    int clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;

    list_pkt list = {NULL, NULL, 0, 0};
    char ab_path[BUF_SIZE];
    strcpy(ab_path, get_absolute_path("."));
    printf("절대경로: %s\n", ab_path);

    list_files_folders(get_absolute_path("."), &list);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    // 전송

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_adr_sz);
        if (clnt_sock == -1)
            error_handling("accept() error");

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        args->clnt_sock = clnt_sock;
        strcpy(args->ab_path, ab_path);
        args->list = list;

        pthread_create(&t_id, NULL, handle_clnt, (void *)args);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }

    close(clnt_sock);
    close(serv_sock);

    // 동적 할당된 메모리 해제
    free(list.file_list);
    free(list.folder_list);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

char *get_absolute_path(char *path)
{
    char *res = realpath(path, buf);
    if (res)
    {
        return buf;
    }
    else
    {
        perror("realpath");
        exit(1);
    }
}

int list_files_folders(char *path, list_pkt *list)
{
    DIR *dir;
    struct dirent *entry;
    int error;

    list->file_count = 0;
    list->folder_count = 0;

    if ((dir = opendir(path)) == NULL)
    {

        error = 0;
        return error;
    }
    else
    {
        error = 1;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            {
                continue;
            }
            if (entry->d_type == DT_REG)
            {
                list->file_count++;
            }
            else if (entry->d_type == DT_DIR)
            {
                list->folder_count++;
            }
        }
        closedir(dir);
    }

    list->file_list = (file_pkt *)malloc(list->file_count * sizeof(file_pkt));
    list->folder_list = (folder_pkt *)malloc(list->folder_count * sizeof(folder_pkt));

    if (list->file_list == NULL || list->folder_list == NULL)
    {
        perror("malloc");
        exit(1);
    }

    int file_index = 0;
    int folder_index = 0;

    if ((dir = opendir(path)) == NULL)
    {
        error = 0;
        return error;
    }
    else
    {
        error = 1;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            {
                continue;
            }
            if (entry->d_type == DT_REG)
            {
                strcpy((char *)list->file_list[file_index].filename, entry->d_name);
                char fullpath[PATH_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
                list->file_list[file_index].filesize = get_file_size(fullpath);
                file_index++;
            }
            else if (entry->d_type == DT_DIR)
            {
                strcpy((char *)list->folder_list[folder_index].filename, entry->d_name);
                folder_index++;
            }
        }

        closedir(dir);
    }

    return error;
}

int get_file_size(char *file_path)
{
    struct stat st;
    if (stat(file_path, &st) == 0)
    {
        if (st.st_size > INT_MAX)
        {
            fprintf(stderr, "File size exceeds the maximum value of int\n");
            exit(1);
        }
        return (int)st.st_size;
    }
    else
    {
        perror("stat");
        exit(1);
    }
}

void *handle_clnt(void *arg)
{

    ThreadArgs *args = (ThreadArgs *)arg;
    int clnt_sock = args->clnt_sock;

    printf("file_count:%d\n", args->list.file_count);
    printf("FolderCount:%d\n", args->list.folder_count);

    printf("Files:\n");
    for (int i = 0; i < args->list.file_count; i++)
    {
        printf("File: %s, Size: %u bytes\n", args->list.file_list[i].filename, args->list.file_list[i].filesize);
    }

    printf("Folders:\n");
    for (int i = 0; i < args->list.folder_count; i++)
    {
        printf("Folder: %s\n", args->list.folder_list[i].filename);
    }

    write(clnt_sock, args->ab_path, BUF_SIZE);
    write(clnt_sock, &args->list.file_count, sizeof(int));
    write(clnt_sock, args->list.file_list, args->list.file_count * sizeof(file_pkt));
    write(clnt_sock, &args->list.folder_count, sizeof(int));
    write(clnt_sock, args->list.folder_list, args->list.folder_count * sizeof(folder_pkt));

    while (1)
    {
        char msg[BUF_SIZE];
        int str_len = read(clnt_sock, msg, BUF_SIZE);
        if (str_len == 0) // close request! client가 종료를 보냈을때
        {

            close(clnt_sock);
            printf("closed client: %d \n", clnt_sock);
            break;
        }
        int a = atoi(msg);
        printf("Received from client: %d\n", a);

        // 수도코드
        switch (a)
        {
        case CD:

            read(clnt_sock, args->ab_path, BUF_SIZE);
            printf("이동된 경로: %s\n", args->ab_path);
            int error;

            error = list_files_folders(args->ab_path, &args->list);
            printf("error: %d\n", error);
            if (error == 0)
            {
                write(clnt_sock, &error, sizeof(int));
                char *last_slash = strrchr(args->ab_path, '/');
                if (last_slash != NULL)
                {

                    *(last_slash) = '\0';
                }
                printf("수정된 절대경로: %s\n", args->ab_path);
                break;
            }
            else
            {
                write(clnt_sock, &error, sizeof(int));
            }

            printf("file_count:%d\n", args->list.file_count);
            printf("FolderCount:%d\n", args->list.folder_count);

            printf("Files:\n");
            for (int i = 0; i < args->list.file_count; i++)
            {
                printf("File: %s, Size: %u bytes\n", args->list.file_list[i].filename, args->list.file_list[i].filesize);
            }

            printf("Folders:\n");
            for (int i = 0; i < args->list.folder_count; i++)
            {
                printf("Folder: %s\n", args->list.folder_list[i].filename);
            }

            write(clnt_sock, &args->list.file_count, sizeof(int));
            write(clnt_sock, args->list.file_list, args->list.file_count * sizeof(file_pkt));
            write(clnt_sock, &args->list.folder_count, sizeof(int));
            write(clnt_sock, args->list.folder_list, args->list.folder_count * sizeof(folder_pkt));
            break;

        case LS:
            printf("done ls\n\n");
            break;

        case DL:
            char recv_ab_file[BUF_SIZE];
            read(clnt_sock, recv_ab_file, BUF_SIZE);
            printf("%s\n\n", recv_ab_file);
            int read_cnt = 0;
            
            FILE *fp;
            fp = fopen(recv_ab_file, "rb");
            if (fp == NULL)
            {
                error=0;
                write(clnt_sock, &error, sizeof(int));
                printf("file operror\n");
                break;
            }
            else{
                error=1;
                write(clnt_sock, &error, sizeof(int));
                char buf[BUF_SIZE];

            while (1)
            {
                read_cnt = fread((void *)buf, 1, BUF_SIZE, fp);
                if (read_cnt < BUF_SIZE)
                {
                    write(clnt_sock, buf, read_cnt);
                    break;
                }
                write(clnt_sock, buf, BUF_SIZE);
                // sleep(1);
            }

            fclose(fp);
            char msg[BUF_SIZE];
            read(clnt_sock, msg, BUF_SIZE);
            printf("Message from client: %s \n", msg);
            break;
            }
            

        case UP:
            int filesize;

            char dl_ap_filename[BUF_SIZE];
            read(clnt_sock, &filesize, sizeof(int));
            printf("filesize: %d\n", filesize);
            read(clnt_sock, dl_ap_filename, BUF_SIZE);
            printf("dl_ap_filename: %s\n", dl_ap_filename);
            FILE *fp1;
            int read_cnt1;

            fp1 = fopen(dl_ap_filename, "wb");
            int total_received = 0;
            while ((read_cnt1 = read(clnt_sock, buf, BUF_SIZE)) != 0)
            {
                total_received += read_cnt1;

                fwrite((void *)buf, 1, read_cnt1, fp1);
                printf("%d\n", total_received);
                if (total_received >= filesize)
                {
                    break;
                }
            }
            fclose(fp1);
            puts("Received file data");
            write(clnt_sock, "Thank you", BUF_SIZE);

            break;
        }
        memset(msg, 0, BUF_SIZE);
    }

    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) // remove disconnected client
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
