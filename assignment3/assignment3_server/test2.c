#include <stdio.h>
#include <stdlib.h>

int main() {
    // 파일 경로 설정
    const char *filePath = "restricted_file.c";
    
    // 파일 포인터 선언
    FILE *file;
    
    // 파일 열기
    file = fopen(filePath, "r");
    if (file == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // 파일 내용 읽고 출력
    int c;
    while ((c = fgetc(file)) != EOF) {
        putchar(c);
    }

    // 파일 닫기
    fclose(file);
    return EXIT_SUCCESS;
}
