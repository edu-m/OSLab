#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#define MAX_LEN 1024
#define UPPER 0
#define LOWER 1
#define REPLACE 2

typedef struct mem{
    char line[MAX_LEN];
    bool eof;
}Mem;

void error(char *what){
    fprintf(stderr,"%s\n",what);
    exit(1);
}

bool doesFileExist(char *filename){
    struct stat file;
    if(stat(filename,&file) || !S_ISREG(file.st_mode))
        return false;
    return true;
}

int init_shm(){
    int shm_des;
    if((shm_des=shmget(IPC_PRIVATE,sizeof(Mem),IPC_CREAT | 0660))==-1)
        error("shmget");
    return shm_des;
}

int init_sem(int child_count){
    int sem_des;
    if((sem_des=semget(IPC_PRIVATE,child_count,IPC_CREAT | 0660))==-1)
        error("semget");
    if(semctl(sem_des,P,SETVAL,1)==-1)
        error("semctl P");
        for(int i=0;i<child_count;i++)
            if(semctl(sem_des,i+1,SETVAL,0)==-1)
                error("semctl F%d",i);
    
    return sem_des;
}

Mem *get_shm(int shm_des){
    Mem *mem;
    if((mem=(Mem *)shmat(shm_des,NULL,0))==(Mem *)-1)
        error("shmat");
    return mem;
}

// uppercase (^) = 0
// uppercase (_) = 1
// uppercase (%) = 2
int get_filter_type(char *arg){
    if(arg[0] == '^')
        return 0;
    if(arg[0] == '_')
        return 1;
    if(arg[0] == '%')
        return 2;
    return -1;
}

void filter(Mem *mem, int sem_id, char op, char *data){
    int filter_type = get_filter_type(op);
    if(filter_type == -1){
        fprintf(stderr,"Warning: filtro non valido, salto al prossimo\n");
        return;
    }
    switch(filter_type){
        case UPPER:
            
            break;
        case LOWER:
            break;
        case REPLACE:
            break;
    }
}

int main(int argc, char **argv){
    if(argc < 3)
        error("uso filter <file.txt> <filter-1> [filter-2] [...]");
    if(!doesFileExist(argv[1]))
        error("File non esistente o non accessibile");
    int sem_id = init_sem(argc-2);
    int mem_id = init_shm();
    Mem* mem = get_shm(mem_id);
    for(int i=2;i<argc-1;i++)
    {
        if(fork == 0)
        {
            filter(mem, sem_id, argv[i][0], argv[i]+1);
            return 0;
        }
    }
}