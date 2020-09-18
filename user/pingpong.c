#include "kernel/types.h"
#include "user/user.h"

// pipe() is unidirectional therefore,
// for two-way communication between processes, two pipes can be set up,
// one for each direction.

// ''ping-pong'' a byte between two processes over a pair of pipes
int main(int argc, char* argv[]) {
    char buf[1];

    int fds_p_c[2];
    // should check err
    int st_p_c = pipe(fds_p_c);

    int fds_c_p[2];
    // should check err
    int st_c_p = pipe(fds_c_p);

    if (st_p_c != 0 || st_c_p != 0) { exit(-1); }

    int pid = fork();
    if (pid == 0) {
        close(fds_p_c[1]);
        read(fds_p_c[0], buf, 1);
        printf("%d: received ping\n", getpid());
        close(fds_p_c[0]);

        close(fds_c_p[0]);
        write(fds_c_p[1], "b", 1);
        close(fds_c_p[1]);
    } else {
        // The parent should send a byte to the child;
        // close the reading end of pipe
        close(fds_p_c[0]);
        // write the string though writing end of the pipe
        write(fds_p_c[1], "b", 1);
        close(fds_c_p[1]);

        close(fds_c_p[1]);
        read(fds_c_p[0], buf, 1);
        close(fds_c_p[0]);

        printf("%d: received pong\n", getpid());
    }

    exit(0);
}