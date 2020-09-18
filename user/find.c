#include "kernel/types.h"

#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

char* basename(char* path) {
    char* p = path + strlen(path);

    // Find first character after last slash.
    for (; p >= path && *p != '/'; p--) {}

    p++;
    return p;
}

void find(char* path, char* name) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
        case T_FILE: {
            if (strcmp(basename(path), name) == 0) { printf("%s\n", path); }
            break;
        }

        case T_DIR: {
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("ls: path too long\n");
                break;
            }

            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                // ? whats this
                if (de.inum == 0) continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;

                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, name);
            }
            break;
        }
    }
    close(fd);
}

int main(int argc, char* argv[]) {
    int i;

    if (argc < 3) { exit(-1); }

    for (i = 2; i < argc; i++) find(argv[1], argv[i]);

    exit(0);
}
