#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    // FILE *f = fopen()
    char *str = malloc(100);
    char d[] = "hello world";
    int fd = open("/fuck", O_RDWR);
    strcpy(str, d);
    printf("%p %p %p %p %p\n", O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_DIRECTORY);
}