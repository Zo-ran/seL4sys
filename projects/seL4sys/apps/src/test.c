#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>

int main() {
    char *str = (char *)malloc(8 * sizeof(char));
    str[0] = 'f';
    str[1] = 'u';
    str[2] = 'c';
    str[3] = 'k';
    str[4] = '\n';
    printf("%s", str);

}