#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char buf[4096] = "abcdefg";
char ret[4096];

void fs_test() {
    remove("/fuck.txt");
    int fd = open("/fuck.txt", O_RDWR | O_CREAT);
    int c = write(fd, buf, 10);
    printf("writelen: %d\n", c);
    
    int rd = open("/fuck.txt", O_RDONLY);
    lseek(rd, -5, SEEK_END);
    int n = read(rd, ret, 100);
    printf("read: %s, n: %d\n", ret, n);
    printf("finish\n");
}

// void vm_test() {
//     char *str1 = malloc(100);
//     strcpy(str1, "hello1\0");
//     malloc(1024);malloc(1024);malloc(1024);malloc(1024);
//     char *str2 = malloc(10);
//     strcpy(str2, "hello2\0");

//     printf("finishasdsad: %s%s\n", str1, str2);
//     printf("finish\n");
// }

void stack_test() {
    char *str = (void *)(0xb0000000);
    strcpy(str, "hello1\0");
    printf("%s\n", str);
}

int main() {
    // remove("/fuck.txt");
    // open("/2.txt", O_CREAT);
    stack_test();
}