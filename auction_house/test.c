#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int getRandom(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}

int main()
{
    srand(time(NULL));
    for (int i = 0; i < 100; i++)
    {
        printf("%d\n", getRandom(1000, 2000));
    }
}