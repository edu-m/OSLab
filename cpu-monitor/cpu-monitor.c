#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#define MAX_BUF 64
#define MSG_BUF 16
#define RAW 1
#define DELTA 2

typedef struct msg{
    long mtype;
    char data[MSG_BUF];
}Msg;

void error(char *what){
    fprintf(stderr,"%s\n",what);
    exit(1);
}

int get_msgq(){
    int msg_q;
    if(msg_q = msgget(IPC_PRIVATE,IPC_CREAT | 0660) == -1)
        error("in getmsg");
    return msg_q;
}

int *getValues(char *line){
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
    return data;
}

void sampler(int samples, int msg_q){
    FILE *stat;
    char *line;
    Msg *tosend = (Msg*)malloc(sizeof(Msg));
    
    for(int i=0;i<samples;i++){
        stat = fopen("/proc/stat","r");
        fgets(line,MAX_BUF,stat);
        int *data = getValues(line);
        printf("%d %d %d\n",data[0], data[1], data[2]);
        tosend->mtype = RAW;
        sprintf(tosend->data,"%d;%d;%d", data[0], data[1], data[2]);
        msgsnd(msg_q,tosend,strlen(tosend->data)+1,0);
        fclose(stat);
    }
    sprintf(tosend->data,"-1;-1;-1");
    msgsnd(msg_q,tosend,strlen(tosend->data)+1,0);
}

void analyzer(int msg_q){
    Msg rcvd;
    while(1){
        msgrcv(msg_q,&rcvd,)
        int first = atoi();
        int second = atoi();
        int third = atoi();
        if(atoi())
    }
}

int main(int argc, char **argv){

    int samples = 30;
    if(argc > 1)
        samples = atoi(argv[1]);
    // inizializziamo coda messaggi
    int msg_q = get_msgq();

    // istanza di processi
    if(fork() == 0) {
        sampler(samples, msg_q);
        return 0;}
    if(fork() == 0) {
        //analyzer(msg_q);
        return 0;}
    if(fork() == 0) {
        //plotter(msg_q);
        return 0;}
    // pulizia coda ipc
    wait(NULL);
    wait(NULL);
    wait(NULL);
    msgctl(msg_q,IPC_RMID,NULL);
}