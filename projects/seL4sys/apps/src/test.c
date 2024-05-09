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

void vm_test() {
    char *str = malloc(20);
    str[0] = '1';
    printf("finish\n");
}

int main() {
    // remove("/fuck.txt");
    // open("/2.txt", O_CREAT);
    vm_test();
}