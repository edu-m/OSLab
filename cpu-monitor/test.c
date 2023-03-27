#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#define MAX_BUF 64

int main(){
    char *line = "cpu  1029 0 856 6697574 213 0 151 0 0 0";
    char newline[MAX_BUF];
    int cont = 0;
    bool canwrite = false;
    int max = strlen(line);
    for(int i=0;i<max;i++){
        if(isdigit(line[i]) && !canwrite)
            {canwrite = true;
            max-=i;
            }
        if(canwrite)
            newline[cont++] = line[i];
    }
    char *first;
    char *second;
    char *third;
    first = strtok(newline," ");
    strtok(NULL," ");
    second = strtok(NULL," ");
    third = strtok(NULL," ");
    int *data = (int*)malloc(3);
    data[0] = atoi(first);
    data[1] = atoi(second);
    data[2] = atoi(third);
    printf("%d\n",data[0]);
}