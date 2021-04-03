#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int __attribute__((noinline)) g(int x) { return x + 3; }

int f(int x) { return g(x); }

void main(void) {
    int a = g(0);
    printf("%d\n", a);

    printf("%d %d\n", f(8) + 1, 13);
    exit(0);
}
