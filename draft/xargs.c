#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// remember read() doesn't add '\0' to terminate to make it string (just gives
// raw buffer).

#define MAXARG 1024

int main(int argc, char* argv[]) {
    if (argc < 2) { exit(-1); }

    char* to_be_passed[MAXARG];
    for (int i = 0; i < 1024; i += 1) {}

    for (int i = 1; i < argc; i += 1) {
        to_be_passed[i - 1] = malloc(sizeof(char) * (strlen(argv[i]) + 1));
        strcpy(to_be_passed[i - 1], argv[i]);
    }

    char buf[1024];
    int pos = 0;

    while (read(0, buf + pos, 1)) {
        if (buf[pos] == '\n' || buf[pos] == '\0') {
            buf[pos] = '\0';

            to_be_passed[argc - 1] = malloc(sizeof(char) * (strlen(buf) + 1));
            strcpy(to_be_passed[argc - 1], buf);
            to_be_passed[argc] = malloc(sizeof(char));
            strcpy(to_be_passed[argc], "\0");

            int pid = fork();
            if (pid == 0) { execv(to_be_passed[0], to_be_passed); }

            free(to_be_passed[argc - 1]);
            free(to_be_passed[argc]);

            pos = 0;
        } else {
            pos += 1;
        }
    }

    for (int i = 0; i < argc - 1; i += 1) { free(to_be_passed[i]); }

    exit(0);
}
