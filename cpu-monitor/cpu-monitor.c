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
#define RAW 1
#define DELTA 2

typedef struct {
    long type;
    int user;
    int system;
    int idle;
} Raw_msg;

typedef struct {
    long type;
    float user;
    float system;
} Delta_msg;

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
    Raw_msg tosend;
    
    for(int i=0;i<samples;i++){
        stat = fopen("/proc/stat","r");
        fgets(line,MAX_BUF,stat);
        int *data = getValues(line);
        //printf("%d %d %d\n",data[0], data[1], data[2]);
        tosend.type = RAW;
        tosend.user = data[0];
        tosend.system = data[1];
        tosend.idle = data[2];
        printf("%d %d %d\n",tosend.user,tosend.system,tosend.idle);
        //sprintf(tosend->data,"%d;%d;%d", data[0], data[1], data[2]);
        if (msgsnd(msg_q, &tosend,sizeof(Raw_msg) - sizeof(tosend.type), 0) == -1)
            perror("msgsnd");
        fclose(stat);
        sleep(1);
    }
    tosend.user = -1.0;
    tosend.system = -1.0;
    tosend.idle = -1.0;
    // sprintf(tosend->data,"-1;-1;-1");
    msgsnd(msg_q,&tosend,sizeof(Raw_msg)-sizeof(long),0);
}

void analyzer(int msg_q){
    Raw_msg rcvd;
    Delta_msg tosend;
    while(1){
        if((msgrcv(msg_q,&rcvd,sizeof(Raw_msg) - sizeof(rcvd.type),RAW,0)) == -1)
            perror("in msgrcv");
        int user = rcvd.user;
        if(user == -1)
        {
            tosend.type = DELTA;
            tosend.user = -1.0;
            // sprintf(tosend->data,"-1.0;-1.0");
            msgsnd(msg_q,tosend,sizeof(Delta_msg)-sizeof(long),0);
            break;
        }
        int system = atoi(strtok(NULL,";"));
        int idle = atoi(strtok(NULL,";"));
        int max = user+system+idle;
        float user_perc = ((float)user / (float)max)*100;
        float system_perc = ((float)system / (float)max)*100;
        user_perc = (user_perc*6)/10;
        system_perc = (system_perc*6)/10;
        tosend->type = DELTA;
        //sprintf(tosend->data,"%f;%f",user_perc,system_perc);
        msgsnd(msg_q,tosend,sizeof(Delta_msg)-sizeof(long),0);
    }
}

void plotter(int msg_q){
    Delta_msg rcvd;
    char plot[60];
    char *stats;
    while(1){
        usleep(500);
        memset(plot,'_',60);
        msgrcv(msg_q,&rcvd,MAX_BUF,DELTA,0);

        float system = rcvd.system;
        float user = rcvd.user;
        if(system == -1.0)
            break;
        for(int i=0;i<(int)user;i++)
            plot[i] = '*';
        for(int i=0;i<(int)system;i++)
            plot[i] = '#';
        
        sprintf(stats,"s: %f u: %f",system,user);
        printf("%s %s\n",plot, stats);
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
        analyzer(msg_q);
        return 0;}
    if(fork() == 0) {
        plotter(msg_q);
        return 0;}
    // pulizia coda ipc
    wait(NULL);
    wait(NULL);
    wait(NULL);
    //msgctl(msg_q,IPC_RMID,NULL);
}