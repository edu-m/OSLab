#include <stdio.h>
#include <stdlib.h>

int main()
{
    for (int i = 0; i < 128; i++)
    {
        printf("%d -> %c\n", i, (char)i);
    }
}

// AL: 65 -> 76
// MZ: 77 -> 90
