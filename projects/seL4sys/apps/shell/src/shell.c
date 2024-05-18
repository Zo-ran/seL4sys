// #include <stdio.h>
// #include <unistd.h>
// #include <assert.h>
// #include <time.h>
// #include <fcntl.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/stat.h>
// #include <sys/sysinfo.h>
// #include <signal.h>
// #include <dirent.h>
// #include "syscall.h"

// #define BUF_SIZE 64
// #define MAX_ARGS 8

// int ls(int argc, char **argv) {
//     char cwd[64] = "\0";
//     getcwd(cwd, sizeof(cwd));
//     sys_ls(cwd);
// }

// int pwd(int argc, char **argv) {
//     char cwd[64] = "\0";
//     getcwd(cwd, sizeof(cwd));
//     printf("%s\n", cwd);
// }

// int ps(int argc, char **argv) {
//     sys_ps();
// }

// int second_time(int argc, char **argv) {
//     printf("%d seconds since boot\n", (int)time(NULL));
// }

// int cd(int argc, char **argv) {
//     chdir(argv[1]);
// }

// int exec(int argc, char **argv) {
//     execv(argv[1], argv + 2);
//     sleep(3);
// }

// struct command {
//     char *name;
//     int (*command)(int argc, char **argv);
// };

// struct command commands[] = { 
//     {"pwd", pwd},
//     {"ls", ls},
//     {"dir", ls},
//     {"ps", ps},
//     {"time", second_time},
//     {"cd", cd},
//     {"exec", exec}
// };

// int main() {
//     int done = 0, new = 1, argc = 0, found = 0;
//     char buf[BUF_SIZE] = "\0";
//     char *argv[MAX_ARGS];
//     // ls(0, NULL);
//     // while (1);
//     while (!done) {
//         if (new) 
//             printf("$ ");
//         fflush(stdout);
//         int k = 0;
//         while((buf[k] = getchar()) != '\n') {
//            k++;
//         }
//         buf[k] = 0;

//         argc = 0;
//         char *p = buf;
//         while (*p != '\0') {
//             /* Remove any leading spaces */
//             while (*p == ' ') {
//                 p++;
//             }
//             if (*p == '\0') {
//                 break;
//             }
//             argv[argc++] = p; /* Start of the arg */
//             while (*p != ' ' && *p != '\0') {
//                 p++;
//             }

//             if (*p == '\0') {
//                 break;
//             }

//             /* Null out first space */
//             *p = '\0';
//             p++;
//         }

//         if (argc == 0) {
//             continue;
//         }

//         found = 0;

//         for (int i = 0; i < sizeof(commands) / sizeof(struct command); i++) {
//             if (strcmp(argv[0], commands[i].name) == 0) {
//                 commands[i].command(argc, argv);
//                 found = 1;
//                 break;
//             }
//         }

//         if (found == 0) {
//             argc = 2;
//             argv[1] = argv[0];
//             argv[0] = "exec";
//             exec(argc, argv);
//         }
//     }
//     printf("hello world\n");    
// }

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char buf[512];

int main() {
    memset(buf, 'a', sizeof(buf));
    
    struct timeval tvs,tve;
    gettimeofday(&tvs, NULL);

	for(ssize_t i = 1000; i > 0; i--) {
        write(STDOUT_FILENO, buf, sizeof(buf));
    }

    gettimeofday(&tve, NULL);

    double span = tve.tv_sec-tvs.tv_sec + (tve.tv_usec-tvs.tv_usec)/1000000.0;
    printf("\ntime: %.12f\n",span);
	return 0;
}