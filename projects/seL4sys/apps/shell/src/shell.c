#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <signal.h>

char buf[4096] = "abcdefg";
char ret[4096];

void fs_test() {
    // sleep(3);
    int fd = open("/fuck.txt", O_RDWR | O_CREAT);
    int c = write(fd, buf, 10);
    printf("writelen: %d\n", c);
    
    int rd = open("/fuck.txt", O_RDONLY);
    lseek(rd, -5, SEEK_END);
    int n = read(rd, ret, 100);
    printf("read: %s, n: %d\n", ret, n);
    remove("/fuck.txt");
    printf("finish: %d\n", fd);
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

// void stack_test() {
//     char *str = (void *)(0xb0000000);
//     strcpy(str, "hello1\0");
//     printf("%s\n", str);
// }

void proc_test() {
    // int pid = getpid();
    // exit
    int pid = execl("/benchmark", "benchmark", NULL);
    // pid = execl("/benchmark", "benchmark", NULL);
    // kill(1, 9);
    while (1) {
        char buf[64] = "\0";
        scanf("%s", buf);
        printf("pid: %d %s\n", getpid(), buf);
    }
    // exit(0);
}

void stdio_test() {
    char str[64] = "\0";
    int len = scanf("%s", str);
    printf("read: %s len: %d\n", str, len);
}     

int main() {
    // remove("/fuck.txt");
    // open("/2.txt", O_CREAT);
    proc_test();
    // vm_test();
    // stdio_test();
    // fs_test();
}