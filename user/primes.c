#include "kernel/types.h"
#include "user/user.h"

void printer(int* inputs) {
    close(inputs[1]);
    int buf;

    if (read(inputs[0], &buf, sizeof(int)) == sizeof(int)) {
        int p = buf;
        printf("prime %d\n", p);

        int outputs[2];
        pipe(outputs);

        int pid = fork();
        if (pid == 0) {
            printer(outputs);
        } else {
            close(outputs[0]);
            while (read(inputs[0], &buf, sizeof(int)) == sizeof(int)) {
                if (buf % p != 0) { write(outputs[1], &buf, sizeof(int)); }
            }
            close(outputs[1]);
        }
    }

    close(inputs[0]);

    exit(0);
};

int main(int argc, char* argv[]) {
    int fd[2];
    pipe(fd);

    int pid = fork();
    if (pid == 0) {
        printer(fd);
    } else {
        close(fd[0]);
        for (int i = 2; i <= 1000; i += 1) { write(fd[1], &i, sizeof(int)); }
    }

    close(fd[1]);

    exit(0);
};

// creates a new pipe and records the read and write file descriptors
// in the array fd
// If no data is available, a read on a pipe waits for either data to be
// written or for all file descriptors referring to the write end to be
// closed;
// in the latter case, read will return 0, just as if the end of a data file
// had been reached.

// it’s important for the child to close the write end of the pipe