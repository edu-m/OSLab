#include <stdio.h>
#include <stdlib.h>

int main()
{
    char mosse[3] = {'C', 'F', 'S'};
    for (int i = 0; i < 3; i++)
        printf("%c\n", (mosse[(i + 1) % 3]));
}