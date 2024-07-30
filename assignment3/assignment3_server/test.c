#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

int main() {
    // 폴더 경로 설정
    const char *folderPath = "restricted_folder";
    
    // 디렉토리 포인터 선언
    DIR *dir;
    struct dirent *entry;

    // 폴더 열기
    dir = opendir(folderPath);
    if (dir == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    printf("Contents of %s:\n", folderPath);
    
    // 폴더 내용을 읽고 출력
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    // 폴더 닫기
    closedir(dir);
    return EXIT_SUCCESS;
}
