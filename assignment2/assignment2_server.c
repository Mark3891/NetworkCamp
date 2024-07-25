#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>

#define BUF_SIZE 4096
#define MSG_SIZE 30

typedef struct
{
    unsigned int seq;
    unsigned int filesize;
    unsigned char file_buf[BUF_SIZE];
} file_pkt;

char client_seq[MSG_SIZE];
file_pkt *send_file;
int serv_sock;
struct sockaddr_in clnt_adr;
socklen_t clnt_adr_sz;
int server_seq=0;

void error_handling(char *message);
void signal_handler(int signo);
void send_Pkt(file_pkt *send_file, int serv_sock, struct sockaddr_in clnt_adr, int clnt_adr_sz);
void recv_ACK(file_pkt *send_file, int serv_sock, struct sockaddr_in clnt_adr, int clnt_adr_sz);

int main(int argc, char *argv[])
{
    struct timeval  tv;
	double begin, end;
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;
    char message[MSG_SIZE];
    struct sockaddr_in serv_adr;
    
    struct sigaction act;
    act.sa_handler=signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGALRM,&act,0);

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    // fp = fopen("test.c", "rb");
    fp = fopen("Hightest.jpg", "rb");
    if (fp == NULL)
        error_handling("File open error");
    fseek(fp,0,SEEK_END);

 int size = ftell(fp);
fseek(fp,0,SEEK_SET);


    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    clnt_adr_sz = sizeof(clnt_adr);
    recvfrom(serv_sock, message, MSG_SIZE, 0,
             (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
    printf("%s\n", message);
    // 연결 메세지 받기

    send_file = (file_pkt *)malloc(sizeof(file_pkt));
    if (send_file == NULL)
        error_handling("Memory allocation error");
    
    memset(send_file, 0, sizeof(file_pkt));
    

   gettimeofday(&tv, NULL);
    begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

    while (1)
    {
        read_cnt = fread((void *)buf, 1, BUF_SIZE, fp);

        if (read_cnt > 0)
        {
			memcpy(send_file->file_buf, buf, read_cnt);
        	send_file->filesize = read_cnt;
			send_file->seq=server_seq;
            send_Pkt(send_file, serv_sock, clnt_adr, clnt_adr_sz);
            
        }
        if (read_cnt < BUF_SIZE)
        {
            if (ferror(fp))
            {
                error_handling("fread() error");
            }
            break;
        }
        recv_ACK(send_file, serv_sock, clnt_adr, clnt_adr_sz);
    }
    gettimeofday(&tv, NULL);
    end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    double time=(end - begin) / 1000;
    printf("Execution time %f\n", time);
    printf("Throughput : %fMbps!\n",size/time/1000000*8);

    fclose(fp);
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void signal_handler(int signo)
{
        printf("TimeOut!!  seq:%d!!\n", send_file->seq);
    
}

void send_Pkt(file_pkt *send_file, int serv_sock, struct sockaddr_in clnt_adr, int clnt_adr_sz)
{
	
	sendto(serv_sock, send_file, sizeof(file_pkt), 0,
           (struct sockaddr *)&clnt_adr, clnt_adr_sz);
    printf("Pkt seq : %d\n", send_file->seq);
    alarm(2);

}

void recv_ACK(file_pkt *send_file, int serv_sock, struct sockaddr_in clnt_adr, int clnt_adr_sz){
    int ack_len;
   
    while((ack_len=recvfrom(serv_sock, client_seq, MSG_SIZE, 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz))==-1){
         send_Pkt(send_file, serv_sock, clnt_adr, clnt_adr_sz);
    }
    if (send_file->seq == atoi(client_seq))
    {	
		alarm(0);
        printf("Ack seq : %d\n", atoi(client_seq));
        server_seq++;
       
    }else{
		   printf("error Ack seq : %d\n", atoi(client_seq));
	}


}