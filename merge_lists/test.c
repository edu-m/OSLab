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

char *trim(char *line)
{
    int cont = 0;
    char buf;
    char *trimmed = (char *)(malloc(MAX_BUF));
    for (int i = 0; i < strlen(line); i++)
    {
        ++cont;
        if (isspace(line[i]) == 0)
            trimmed[i] = line[i];
    }
    trimmed[cont + 1] = '\0';
    return trimmed;
}

char *tolower_c(char *data)
{
    char *loweredString = (char *)malloc(MAX_BUF);
    for (size_t i = 0; i < strlen(data) + 1; i++)
        loweredString[i] = tolower(data[i]);
    return loweredString;
}

int main()
{
    char *test = "CIAO";
    printf("%s", tolower_c(test));
}