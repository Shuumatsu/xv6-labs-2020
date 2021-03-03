#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    // fildes[0] represents rx
    // fildes[1] represents tx
    int fildes_c_p[2];
    {
        int status = pipe(fildes_c_p);
        if (status == -1) { exit(-1); }
    }
    int fildes_p_c[2];
    {
        int status = pipe(fildes_p_c);
        if (status == -1) { exit(-1); }
    }

    int pid = fork();
    if (pid == -1) { exit(-1); }
    int curr_pid = getpid();

    if (pid == 0) {
        // child
        close(fildes_p_c[1]);
        close(fildes_c_p[0]);

        char buf[1];
        int n = read(fildes_p_c[0], buf, 1);
        if (n != 1) { exit(-1); }
        close(fildes_p_c[0]);

        printf("%d: received ping\n", curr_pid);

        write(fildes_c_p[1], "", 1);
        close(fildes_c_p[1]);
    } else {
        // parent
        close(fildes_p_c[0]);
        close(fildes_c_p[1]);

        write(fildes_p_c[1], "", 1);
        close(fildes_p_c[1]);

        char buf[1];
        int n = read(fildes_c_p[0], buf, 1);
        if (n != 1) { exit(-1); }
        close(fildes_c_p[0]);

        printf("%d: received pong\n", curr_pid);
    }

    exit(0);
}
