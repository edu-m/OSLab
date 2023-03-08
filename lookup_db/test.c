#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main()
{
    char *test = "Hello";
    printf("%d %d", sizeof(test), strlen(test));
}