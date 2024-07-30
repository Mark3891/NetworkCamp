#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


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

void error_handling(char *message);
void print_listFile(list_pkt *list_file);
int folder_exists(list_pkt *list, const char *folder_name);
int file_exists(list_pkt *list, const char *file_name);
char ab_path[BUF_SIZE];
list_pkt list;
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    int sock;
    struct sockaddr_in serv_adr;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");

    puts("Connected...........");

    memset(&list, 0, sizeof(list_pkt));

    read(sock, ab_path, BUF_SIZE);
    printf("절대경로: %s\n", ab_path);

    int file_count, folder_count;
    read(sock, &file_count, sizeof(int));
    printf("file_count: %d\n", file_count);

    list.file_count = file_count;
    list.file_list = (file_pkt *)malloc(file_count * sizeof(file_pkt));
    if (list.file_list == NULL)
    {
        perror("malloc");
        exit(1);
    }
    int bytes_received = 0;
    while (bytes_received < file_count * sizeof(file_pkt))
    {
        int result = read(sock, (char *)list.file_list + bytes_received, (file_count * sizeof(file_pkt)) - bytes_received);
        if (result < 0)
        {
            error_handling("Failed to read file list");
        }
        bytes_received += result;
    }

    read(sock, &folder_count, sizeof(int));
    printf("folder_count: %d\n", folder_count);

    list.folder_count = folder_count;
    list.folder_list = (folder_pkt *)malloc(folder_count * sizeof(folder_pkt));
    if (list.folder_list == NULL)
    {
        perror("malloc");
        exit(1);
    }

    bytes_received = 0;
    while (bytes_received < folder_count * sizeof(folder_pkt))
    {
        int result = read(sock, (char *)list.folder_list + bytes_received, (folder_count * sizeof(folder_pkt)) - bytes_received);
        if (result < 0)
        {
            error_handling("Failed to read folder list");
        }
        bytes_received += result;
    }

    print_listFile(&list);

    while (1)
    {
        char message[BUF_SIZE];
        fputs("Input Index (1 to CD, 2 to LS, 3 to download, 4 to upload): ", stdout);
        fgets(message, BUF_SIZE, stdin);
        write(sock, message, BUF_SIZE);

        int a = atoi(message);

        // 수도코드
        switch (a)
        {
        case CD:
            char folderName[BUF_SIZE];

            fputs("Input folder name: ", stdout);
            fgets(folderName, BUF_SIZE, stdin);
            folderName[strcspn(folderName, "\n")] = '\0';

            if (strcmp(folderName, "..") == 0)
            {
                char *last_slash = strrchr(ab_path, '/');
                if (last_slash != NULL)
                {

                    *(last_slash) = '\0';
                }
                printf("수정된 절대경로: %s\n", ab_path);
            }
            else if (folder_exists(&list, folderName))
            {
                if (strcmp(ab_path, "/") != 0)
                {
                    if (strlen(ab_path) > 0 && ab_path[strlen(ab_path) - 1] != '/')
                    {
                        strcat(ab_path, "/");
                    }
                }
                strcat(ab_path, folderName);
                printf("수정된 절대경로: %s\n", ab_path);
            }
            else
            {
                printf("Folder does not exist.\n");
                break;
            }
            write(sock, ab_path, BUF_SIZE);
            //opendir error 처리
            int error;
            read(sock, &error, sizeof(int));
            if(error==0){
                printf("permission denied!\nNot Open!\n");
                char *last_slash = strrchr(ab_path, '/');
                if (last_slash != NULL)
                {

                    *(last_slash) = '\0';
                }
                printf("현재 절대경로: %s\n", ab_path);
                break;
            }
            // list_pkt list;
            memset(&list, 0, sizeof(list_pkt));

            int file_count, folder_count;
            read(sock, &file_count, sizeof(int));
            printf("file_count: %d\n", file_count);

            list.file_count = file_count;
            list.file_list = (file_pkt *)malloc(file_count * sizeof(file_pkt));
            if (list.file_list == NULL)
            {
                perror("malloc");
                exit(1);
            }
            int bytes_received = 0;
            while (bytes_received < file_count * sizeof(file_pkt))
            {
                int result = read(sock, (char *)list.file_list + bytes_received, (file_count * sizeof(file_pkt)) - bytes_received);
                if (result < 0)
                {
                    error_handling("Failed to read file list");
                }
                bytes_received += result;
            }

            read(sock, &folder_count, sizeof(int));
            printf("folder_count: %d\n", folder_count);

            list.folder_count = folder_count;
            list.folder_list = (folder_pkt *)malloc(folder_count * sizeof(folder_pkt));
            if (list.folder_list == NULL)
            {
                perror("malloc");
                exit(1);
            }

            bytes_received = 0;
            while (bytes_received < folder_count * sizeof(folder_pkt))
            {
                int result = read(sock, (char *)list.folder_list + bytes_received, (folder_count * sizeof(folder_pkt)) - bytes_received);
                if (result < 0)
                {
                    error_handling("Failed to read folder list");
                }
                bytes_received += result;
            }

            break;

        case LS:
            printf("filecount: %d, foldercount: %d\n\n\n", list.file_count, list.folder_count);
            print_listFile(&list);
            break;

        case DL:

            char fileName[BUF_SIZE];
            char send_fileName[BUF_SIZE];
            memset(send_fileName, 0, BUF_SIZE);

            fputs("Input file name: ", stdout);
            fgets(fileName, BUF_SIZE, stdin);
            fileName[strcspn(fileName, "\n")] = '\0';

            if (file_exists(&list, fileName))
            {
                strcat(send_fileName, ab_path);
                strcat(send_fileName, "/");
                strcat(send_fileName, fileName);
                printf("절대경로: %s\n", ab_path);
                printf("다운받을 파일 절대경로: %s\n", send_fileName);
            }
            else
            {
                printf("파일 없음!\n");
                break;
            }
            write(sock, send_fileName, BUF_SIZE);
           
            read(sock,&error,sizeof(int));
            if(error==0){
                printf("permission denied!\nNot Open!\n");
                printf("현재 절대경로: %s\n", ab_path);
                break;

            }

            int filesize;
            for (int i = 0; i < list.file_count; i++)
            {
                if (strcmp((char *)list.file_list[i].filename, fileName) == 0)
                {
                    filesize = list.file_list[i].filesize;
                    printf("%d\n\n\n", filesize);
                }
            }
            FILE *fp;
            int read_cnt;

            fp = fopen(fileName, "wb");
            int total_received = 0;
            while ((read_cnt = read(sock, buf, BUF_SIZE)) != 0)
            {
                total_received += read_cnt;

                fwrite((void *)buf, 1, read_cnt, fp);
                printf("%d\n", total_received);
                if (total_received >= filesize)
                {
                    break;
                }
            }
            fclose(fp);
            puts("Received file data");
            write(sock, "Thank you", BUF_SIZE);
            break;

        case UP:

            char up_fileName[BUF_SIZE];
            char upload_fileName[BUF_SIZE];
            memset(upload_fileName, 0, BUF_SIZE);
            fputs("Input file name: ", stdout);
            fgets(up_fileName, BUF_SIZE, stdin);
            up_fileName[strcspn(up_fileName, "\n")] = '\0';

            strcat(upload_fileName, ab_path);
            strcat(upload_fileName, "/");
            strcat(upload_fileName, up_fileName);
            printf("절대경로: %s\n", ab_path);
            printf("보낼 파일 절대경로: %s\n", upload_fileName);
            int up_filesize;
            FILE *fp1;
            fp1 = fopen(up_fileName, "rb");

            fseek(fp1, 0, SEEK_END);
            up_filesize = ftell(fp1);
            fseek(fp1, 0, SEEK_SET);
            write(sock, &up_filesize, sizeof(int));
            upload_fileName[strcspn(upload_fileName, "\n")] = '\0';
            write(sock, upload_fileName, BUF_SIZE);

           
            char buf[BUF_SIZE];
            read_cnt=0;
            while (1)
            {
                read_cnt = fread((void *)buf, 1, BUF_SIZE, fp1);
                if (read_cnt < BUF_SIZE)
                {
                    write(sock, buf, read_cnt);
                    break;
                }
                write(sock, buf, BUF_SIZE);
                // sleep(1);
            }

            fclose(fp1);
            char msg[BUF_SIZE];
            read(sock, msg, BUF_SIZE);
            printf("Message from server: %s \n", msg);

            break;
        }
        memset(message, 0, BUF_SIZE);
    }

    close(sock);

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

void print_listFile(list_pkt *list_file)
{
    printf("Files:\n");
    for (int i = 0; i < list_file->file_count; i++)
    {
        printf("Filename: %s, Size: %u bytes\n", list_file->file_list[i].filename, list_file->file_list[i].filesize);
    }

    printf("Folders:\n");
    for (int i = 0; i < list_file->folder_count; i++)
    {
        printf("Foldername: %s\n", list_file->folder_list[i].filename);
    }
}

int folder_exists(list_pkt *list, const char *folder_name)
{
    for (int i = 0; i < list->folder_count; i++)
    {
        if (strcmp((char *)list->folder_list[i].filename, folder_name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int file_exists(list_pkt *list, const char *file_name)
{
    for (int i = 0; i < list->file_count; i++)
    {
        if (strcmp((char *)list->file_list[i].filename, file_name) == 0)
        {
            return 1;
        }
    }
    return 0;
}