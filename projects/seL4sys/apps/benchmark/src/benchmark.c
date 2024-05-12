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


int main(int argc, char **argv) {
    // while (1) {
    //     printf("%s: Hello world!\n", argv[0]);
    // }
    while (1) {
        char buf[64] = "\0";
        scanf("%s", buf);
        printf("pid: %d %s\n", getpid(), buf);
    }
}