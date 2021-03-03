#include "kernel/types.h"

#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    if (argc < 2) { exit(-1); }

    char buf[512];
    int pos = 0;
    while (read(0, buf + pos, 1) > 0) {
        if (buf[pos] != '\n' && buf[pos] != '\0') {
            pos += 1;
            continue;
        }
        buf[pos] = '\0';

        int pid = fork();
        if (pid == -1) { exit(-1); }

        if (pid == 0) {
            char* to_be_passed[argc];
            for (int i = 1; i < argc; i += 1) { to_be_passed[i - 1] = argv[i]; }
            to_be_passed[argc - 1] = buf;

            exec(to_be_passed[0], to_be_passed);

            printf("not reachable\n");
        } else {
            wait(0);
            pos = 0;
        }
    }

    exit(0);
}
