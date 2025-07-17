#include <stdio.h>

int main (int argc, const char* const argv[])
{
    for (int i = 0; i < argc; ++i) {
        printf("%d: %s\n", i, argv[i]);
    }
    return 0;
}
