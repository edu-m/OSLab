#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_BUF 65 // sizeof(long) + 1

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

enum SEM_TYPE {MNG = 0, ADD, MUL, SUB};

typedef struct shm{
    long currop;
    long partial;
    bool eof;
}Mem;

bool doesFileExist(char *filename){
    struct stat file;
    if(stat(filename,&file) || !S_ISREG(file.st_mode))
        return false;
    return true;
}

void error(char *what){
    fprintf(stderr, "%s\n",what);
    exit(1);
}

int get_shm_id(){
    int shm_id;
    if((shm_id = shmget(IPC_PRIVATE,sizeof(Mem),IPC_CREAT | 0660)) == -1)
        error("in shmget");
    return shm_id;
}

Mem *get_shm_mem(int shm_id){
    Mem *mem;
    if((mem = (Mem*)shmat(shm_id,NULL,0)) == MAP_FAILED)
        error("in shmat");
    return mem;
}

int get_sem(){
    int sem_id;
    if((sem_id = semget(IPC_PRIVATE,4,IPC_CREAT | 0660)) == -1)
        error("in semget");

    if(semctl(sem_id,MNG,SETVAL,1))
        error("in semctl MNG");
    if(semctl(sem_id,ADD,SETVAL,0))
        error("in semctl ADD");
    if(semctl(sem_id,MUL,SETVAL,0))
        error("in semctl MUL");
    if(semctl(sem_id,SUB,SETVAL,0))
        error("in semctl SUB");

    return sem_id;
}

void op(int id, Mem* mem, int sem_id){
    while(1){
        WAIT(sem_id,id);
        if(mem->eof)
            break;
        switch(id){
            case ADD:
                printf("ADD: %ld+%ld\n",mem->partial,mem->currop);
                mem->partial+=mem->currop;
                break;
            case MUL:
                printf("MUL: %ld*%ld\n",mem->partial,mem->currop);
                mem->partial*=mem->currop;
                break;
            case SUB:
                printf("SUB: %ld-%ld\n",mem->partial,mem->currop);
                mem->partial-=mem->currop;
                break;
        }
        SIGNAL(sem_id,MNG);
    }
}

char *trim(char *line)
{
    size_t i;
    char *trimmed = (char *)(malloc(MAX_BUF));
    for (i = 0; i < strlen(line); i++)
        if (isspace(line[i]) == 0)
            trimmed[i] = line[i];
    trimmed[i + 1] = '\0';
    return trimmed;
}

// enum SEM_TYPE {MNG = 0,ADD,MUL,SUB};

void shift_line(char *input){
    for(int i=0;i<strlen(input);i++)
        input[i] = input[i+1];
}

void mng(int sem_id, Mem* mem, char* filename){
    FILE *file = fopen(filename,"r");
    char line[MAX_BUF];
    char op;
    int target;
    while(fgets(line,MAX_BUF,file)){
        WAIT(sem_id,MNG);
        op = line[0];
        //memmove (line, &line[1], strlen(line));
        shift_line(line);
        printf("MNG: risultato intermedio: %ld; letto %s\n",mem->partial,trim(line));
        mem->currop = atol(line);
        if(op == '+')      target = ADD;
        else if(op == '*') target = MUL;
        else if(op == '-') target = SUB;
        // printf("sveglio %d\n",target);
        SIGNAL(sem_id,target);
    }
    usleep(50);
    mem->eof=true;
    for(int i=1;i<4;i++)
        SIGNAL(sem_id,i);
    printf("Risultato finale %ld\n",mem->partial);
}   

int main(int argc, char **argv){
    if(argc<2)
        error("uso: calculator <list.txt>");
    if(!doesFileExist(argv[1]))
        error("file non esistente o non valido");
    
    int shm_id = get_shm_id();
    Mem* mem = get_shm_mem(shm_id);
    mem->partial=0;
    mem->eof = false;
    int sem_id = get_sem();

   if(fork() == 0){
        mng(sem_id,mem,argv[1]);
        return 0;}
    if(fork() == 0){
        op(1,mem,sem_id);
        return 0;}
    if(fork() == 0){
        op(2,mem,sem_id);
        return 0;}
    if(fork() == 0){
        op(3,mem,sem_id);
        return 0;}
 
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    shmctl(shm_id,IPC_RMID,NULL);
    semctl(sem_id,0,IPC_RMID);
}