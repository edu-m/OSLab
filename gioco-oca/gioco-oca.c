#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <sys/shm.h>
#define MAX_PLAYERS 8
#define MAX_BUF 10
#define J 0
#define P 1

void error(char *what){
    fprintf(stderr,"%s", what);
    exit(1);
}

typedef struct mem{
    int target;
    int step[MAX_PLAYERS];
}Mem;

int WAIT(int sem_des, int num_semaforo)
{
    struct sembuf operazioni[1] = {{num_semaforo, -1, 0}};
    return semop(sem_des, operazioni, 1);
}
int SIGNAL(int sem_des, int num_semaforo)
{
    struct sembuf operazioni[1] = {{num_semaforo, +1, 0}};
    return semop(sem_des, operazioni, 1);
}

int get_mem_id()
{
    int mem_id;
    if((mem_id = shmget(IPC_PRIVATE,sizeof(Mem),IPC_CREAT | 0660)) == -1)
        error("in shmget");
}

Mem *get_mem(int mem_id){
    Mem *mem;
    if ((mem = (Mem*)shmat(mem_id,NULL,0)) == MAP_FAILED)
        error("in shmat");
    return mem;
}

int get_sem(int players){
    int sem_id;
    if((sem_id = semget(IPC_PRIVATE,2,IPC_CREAT | 0660)) == -1)
        error("in msgget");
    if((semctl(sem_id,J,SETVAL,players)) == -1)
        error("in semctl J");
    if((semctl(sem_id,P,SETVAL,0)) == -1)
        error("in semctl P");
    return sem_id;
}

int getMinSteps(int *step, int target){
int min = target;
int index;
for(int i=0;i<MAX_PLAYERS;i++)
    if(step[i] < min)
        {
            min = step[i];
            index = i;
        }
return index;
}

void judge(int sem_id, Mem *mem, char *filename, int players){
    FILE *f = fopen(filename, "r");
    char *line;
    while(!feof(f)){
        fgets(line,MAX_BUF,f);
        for(int i=0; i<players;i++)
            WAIT(sem_id, J);
        mem->target = atoi(line);
        memset(line,0,MAX_BUF);
         for(int i=0; i<players;i++)
            SIGNAL(sem_id, P);
        printf("P%d vince\n",getMinSteps(mem->step, mem->target));
    }
    mem->target = -1;
}

void player(int id, int sem_id, Mem *mem){
    int cont = 0;
    int i = 0;
    srand(time(NULL)*50);
    
    while(1){
    WAIT(sem_id,P);
    if(mem->target == -1) 
        break;
    while(cont < mem->target)   
        {
            cont += rand() % mem->target + 1;
            ++i;
        }
        mem->step[id] = i;
        SIGNAL(sem_id,J);
    }
}

int main(int argc, char **argv){
    if(argc<3)
        error("uso: gioco-oca <file.txt> <n-giocatori>");
    int players = atoi(argv[2]);
    if(players < 2 || players > 8)
        error("inserire un numero compreso tra 2 e 8 giocatori");

    int sem_id = get_sem(players);
    int mem_id = get_mem_id();
    Mem *mem = get_mem(mem_id);

    if(fork() == 0){
        judge(sem_id, mem, argv[1], players);
        return 0;
    }

    for(int i=0;i<players-1;i++){
        if(fork() == 0){
            player(i, sem_id, mem);
            return 0;
        }
    }

    shmctl(mem_id,IPC_RMID,NULL);
    semctl(sem_id,0,IPC_RMID);
}