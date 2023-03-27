#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    char s[] = "*527";
    memmove (s, s+1, strlen (s+1) + 1); // or just strlen (s)
    printf("%d\n",atoi(s));
}