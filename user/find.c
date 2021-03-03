#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

char* basename(char* path) {
    char* p = path + strlen(path);

    // Find first character after last slash.
    for (; p >= path && *p != '/'; p--) {}

    p++;
    return p;
}

int find(char* path, char* target) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) { return -1; }

    struct stat st;
    if (fstat(fd, &st) == -1) { return -1; }

    switch (st.type) {
        case T_DIR: {
            char buf[MAXPATH];
            memset(buf, 0, MAXPATH);
            strcpy(buf, path);
            *(buf + strlen(path)) = '/';
            char* p = buf + strlen(path) + 1;

            struct dirent de;
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || strcmp(de.name, ".") == 0 ||
                    strcmp(de.name, "..") == 0) {
                    continue;
                }

                int name_len = 0;
                for (int i = 0; i < DIRSIZ && de.name[i] != '\0'; i += 1) {
                    name_len += 1;
                }

                if (strlen(path) + 1 + name_len + 1 >= MAXPATH) {
                    printf("ls: path too long\n");
                    return -1;
                }

                memmove(p, de.name, name_len);
                *(p + name_len) = '\0';

                if (find(buf, target) == -1) { return -1; }
            }

            break;
        }
        case T_FILE: {
            if (strcmp(basename(path), target) == 0) { printf("%s\n", path); }
            break;
        }
        case T_DEVICE: {
            break;
        }
        default: {
            return -1;
        }
    }

    close(fd);

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) { exit(-1); }

    char* root = argv[1];
    char* target = argv[2];

    int status = find(root, target);
    if (status == -1) { exit(-1); }

    exit(0);
}