#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define MAX_BUF 2048
enum{P,L};

typedef struct{
    char line[MAX_BUF];
    bool eof;
}shm_data;


int WAIT(int sem_des, int num_semaforo){
    printf("Entra");
struct sembuf operazioni[1] = {{num_semaforo,-1,0}};
return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo){
struct sembuf operazioni[1] = {{num_semaforo,+1,0}};
return semop(sem_des, operazioni, 1);
}
void error(char *what){
    fprintf(stderr,"%s\n",what);
    exit(1);
}

bool does_file_exist(char *path){
    struct stat s;
    if(stat(path,&s)==-1 || !S_ISREG(s.st_mode))
        return false;
    return true;
}

int init_shm(){
    int shm_des;
    if((shm_des=shmget(IPC_PRIVATE,sizeof(shm_data),IPC_CREAT | 0660))==-1)
        error("shmget");
    return shm_des;
}

shm_data *get_shm(int shm_des){
    shm_data *mem;
    if((mem=(shm_data *)shmat(shm_des,NULL,0))==(shm_data *)-1)
        error("shmat");
    return mem;
}

int init_sem(){
    int sem_des;
    if((sem_des=semget(IPC_PRIVATE,2,IPC_CREAT | 0660))==-1)
        error("semget");
    //return sem_des;

    if(semctl(sem_des,P,SETVAL,1)==-1)
        error("semctl P");
    if(semctl(sem_des,L,SETVAL,0)==-1)
        error("semctl L");
    return sem_des;
}

int init_msg(){
    int msg_des;
    if((msg_des=msgget(IPC_PRIVATE,IPC_CREAT | 0660))==-1)
        error("msgget");
    return msg_des;
}


int main(int argc,char *argv[]){
    if(argc<2)
        error("Uso <file.txt>");
    if(!does_file_exist(argv[1]))
        error("Il file non esiste");
    
    int shm_des=init_shm();
    shm_data *mem;
    mem=get_shm(shm_des);
    int msg_des=init_msg();
    int sem_des=init_sem();
   

    mem->eof=false;
    FILE *f;
    if((f=fopen(argv[1],"r"))==NULL)
        error("Errore nell'apertura del file");
    char line[MAX_BUF];

     while(fgets(line,MAX_BUF,f)){
        WAIT(sem_des,P);
        printf("ciao\n");
        printf("%s\n",line);
        strncpy(mem->line,line,MAX_BUF);
        printf("%s\n",mem->line);
        for(int i=0;i<26;i++)
            SIGNAL(sem_des,L);
    }
    mem->eof=true;

    for(int i=0;i<26;i++){//Creazione processi L
        if(fork()==0){
            //letter();
            return 0;
        }
    }

    if(fork()==0){
        //statistics();
        return 0;
    }

    for(int i=0;i<27;i++)
        wait(NULL);
    
    shmctl(shm_des,IPC_RMID,NULL);
    semctl(sem_des,P,IPC_RMID,0);
    semctl(sem_des,L,IPC_RMID,0);
    msgctl(msg_des,IPC_RMID,NULL);

    return 0;
}