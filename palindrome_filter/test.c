#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#define MAX_BUF 32
char *trim(char *line)
{
    size_t i;
    size_t j=0;
    char *trimmed = (char *)(malloc(MAX_BUF));
    for (i = 0; i < strlen(line); i++)
    {
        if (!isspace(line[i]))
            trimmed[j++] = line[i];
    }
    trimmed[j + 1] = '\0';
    return trimmed;
}

int main(){
    printf("%s\n",trim("    ci      ao       "));
}