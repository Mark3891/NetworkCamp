#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 4096
#define MSG_SIZE 30

typedef struct
{
	unsigned int seq;
	unsigned int filesize;
	unsigned char file_buf[BUF_SIZE];
} file_pkt;

void error_handling(char *message);

int main(int argc, char *argv[])
{

	
	int sock;
	char buf[BUF_SIZE];
	int read_cnt;
	int str_len;
	socklen_t adr_sz;
	FILE *fp;
	// fp = fopen("receive.c", "wb");
	fp = fopen("receive.jpg", "wb");
	file_pkt *recv_file;

	struct sockaddr_in serv_adr, from_adr;
	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));
	char message[MSG_SIZE] = "connect!!";
	recv_file = (file_pkt *)malloc(sizeof(file_pkt));
	memset(recv_file, 0, sizeof(file_pkt));

	sendto(sock, message, MSG_SIZE, 0,
		   (struct sockaddr *)&serv_adr, sizeof(serv_adr));
	adr_sz = sizeof(from_adr);

	int client_seq = 0;

	while ((read_cnt = recvfrom(sock, recv_file, sizeof(file_pkt), 0, (struct sockaddr *)&from_adr, &adr_sz)) != 0)
	{
		if (read_cnt == -1)
			error_handling("read() error");
		
		if (recv_file->seq == client_seq)
		{
			fwrite(recv_file->file_buf, 1, recv_file->filesize, fp);
			sprintf(buf,"%d",client_seq); 
			sendto(sock, buf, MSG_SIZE, 0,(struct sockaddr *)&serv_adr, sizeof(serv_adr));
			client_seq++;
		}
		else if(recv_file->seq!=client_seq){
			sprintf(buf,"%d",client_seq-1); 
			sendto(sock,buf, MSG_SIZE, 0,
				   (struct sockaddr *)&serv_adr, sizeof(serv_adr));
		}

		if (recv_file->filesize != BUF_SIZE)
			break;
	}
	fclose(fp);

	close(sock);

	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}