#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    printf("argc: %d\n", argc);
    for (int i = 0; i < argc; i += 1) { printf("%s\n", argv[i]); }
    return 0;
}