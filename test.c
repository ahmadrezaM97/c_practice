#include <stdio.h>

int main() {
    FILE* fp = fopen("files/1.txt", "r"); // relative path
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }
    char buf[10];
    int n = fread(buf, 1, sizeof buf, fp);
    if (n > 1) {
        printf("%s", buf);
    }


    // use the file
    fclose(fp);
    return 0;
}