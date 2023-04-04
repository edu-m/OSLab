#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#define MAX_BUF 32

int main()
{
    char *test = "CIAO";
    int *k = (int *)calloc(1, sizeof(int));
    printf("%d\n", *k);
    free(k);
    free(k); // free(): double free detected in tcache 2
}