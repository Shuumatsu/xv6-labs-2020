#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void printer(int infilde) {
    int buf;
    if (read(infilde, &buf, sizeof(int)) == sizeof(int)) {
        int p = buf;
        printf("prime %d\n", p);

        int outfilde[2];
        int status = pipe(outfilde);
        if (status == -1) { exit(-1); }

        int pid = fork();
        if (pid == -1) { exit(-1); }

        if (pid == 0) {
            // child
            close(infilde);

            close(outfilde[1]);
            printer(outfilde[0]);
        } else {
            close(outfilde[0]);

            while (read(infilde, &buf, sizeof(int)) == sizeof(int)) {
                if (buf % p != 0) { write(outfilde[1], &buf, sizeof(int)); }
            }
            close(infilde);
            close(outfilde[1]);
        }
    }

    exit(0);
}

int main(int argc, char* argv[]) {
    int fildes[2];
    int status = pipe(fildes);
    if (status == -1) { exit(-1); }

    int pid = fork();
    if (pid == -1) { exit(-1); }

    if (pid == 0) {
        close(fildes[1]);
        printer(fildes[0]);
    } else {
        close(fildes[0]);
        for (int i = 2; i <= 1000; i += 1) {
            write(fildes[1], &i, sizeof(int));
        }
        close(fildes[1]);
    }

    exit(0);
}
