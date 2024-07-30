#assignment3

CD 확인
client -> 1 누르고 -> 가고자하는 폴더 입력 --> 현재 이동한 절대위치를 알 수 있다.

LS 확인
client -> 2 누른다. 지금 속해있는 위치를 알려주고 위치 안에 있는 파일 및 폴더 다 알려준다.

DL(다운로드)
1) client -> 3 누른다. -> 이동해서 현재 속해 있는 파일을 입력한다.
2) client -> 3 누른다. -> 처음 위치에 restricted_file.c 를 입력한다. --> 접근할 수 없는 파일 에러 처리 확인을 위해서

UP(업로드)
client -> 4 누른다. -> 보내고자 하는 파일을 입력한다. --> 서버에서 내가 이동한 위치에 파일이 업로드 됐는지 확인한다.

#assignment4


client가 ncurses를 사용했기 때문에 컴파일 방법 컴퓨터마다 다르다.
현재 내 서버에서는 이런식으로 컴파일하였다.
all: assignment4_client.c
	gcc -o assignment4 assignment4_client.c -I/home/s22100584/ncurses/include/ncurses -L/home/s22100584/ncurses/lib -lncurses

Client에서 대문자, 소문자 및 스페이스바를 입력하면 서버에서 해당 글자를 포함한 데이터들을 sort해서 보내준다.
 



