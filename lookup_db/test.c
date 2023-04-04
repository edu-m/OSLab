#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_BUF 32

char *trim(char *name){
    char *trimmed = (char*)malloc(MAX_BUF);
    int c=0;
    for(int i=0;i<strlen(name);i++){
        if(!isspace(name[i]))
            trimmed[c++]=name[i];
    }
    trimmed[c+1]='\0';
    return trimmed;
}

int main()
{
    char *test = "Hello";
    printf("'%s'\n", trim("  ciao    \n"));
}