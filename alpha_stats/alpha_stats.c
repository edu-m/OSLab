#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#define ALP 0
#define MZP 1

enum PROCESS{P,AL,MZ};
enum LETTERS{a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z};

typedef struct{
    char c;
    bool terminated;

}shm_char;

typedef struct{
    long stats[26];

}shm_stats;

int WAIT(int sem_des, int num_semaforo){
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

bool does_file_exists(char *path){
    struct stat s;
    if(stat(path,&s)==-1 || !S_ISREG(s.st_mode))
        return false;
    return true;

}

int init_shm(char *type){
    int shm_des;
    if((shm_des=shmget(IPC_PRIVATE,sizeof(type),IPC_CREAT | 0660))==-1)
    error("shmget");
    return shm_des;
}

/*type *get_shm(int shm_des,char *type){
    type *mem;
    if((mem=(type *)shmat(shm_des,NULL,0))==(type *)-1)
        error("shmat");
    return mem;
}*/

int init_sem(){
    int sem_des;
    if((sem_des=semget(IPC_PRIVATE,3,IPC_CREAT | 0660))==-1)
        error("semget");
    if(semctl(sem_des,P,SETVAL,1)==-1)
        error("semctl P");
     if(semctl(sem_des,AL,SETVAL,0)==-1)
        error("semctl AL");
     if(semctl(sem_des,AL,SETVAL,0)==-1)
        error("semctl MZ");
    return sem_des;

}

void letter(shm_char *mem_c,shm_stats *mem_s,int sem_des,int id){
    memset(mem_s->stats,0,sizeof(mem_s->stats));
    printf("La funzione letter viene chiamata\n");

    while(1){
        if(!mem_c->terminated){
            printf("finito il ciclo per i processi AL ed MZ");
            break;
        }
        if(id==ALP){
            WAIT(sem_des,AL);
            printf("AL procede\n");
            mem_s->stats[mem_c->c]++;
            SIGNAL(sem_des,P);
        }
        else{
            WAIT(sem_des,MZ);
            printf("MZ procede\n");
            mem_s->stats[mem_c->c]++;
            SIGNAL(sem_des,P);

        }
    }



}
int main(int argc,char *argv[]){
    if(argc<2)
        error("Uso <file.txt>");
    if(!does_file_exists(argv[1]))
        error("Il file non esiste");
    char *path=argv[1];
    
    int shm_des_c=init_shm("shm_char");
    int shm_des_s=init_shm("shm_stats");

    int sem_des=init_sem();

    //mappatura del file in memoria

    int f_des;
    struct stat s;
    stat(path,&s);
    if((f_des=open(path,O_RDWR|O_CREAT,0660))==-1)
        error("open");
    char *mapped;
    if((mapped=mmap(NULL,s.st_size,PROT_READ,MAP_PRIVATE,f_des,0))==MAP_FAILED)
        error("mmap");

    //lettura del file e deposito in memoria condivisa

    shm_char *mem_c;
    if((mem_c=(shm_char *)shmat(shm_des_c,NULL,0))==(shm_char *)-1)
        error("shmat char");
    shm_stats *mem_s;
    if((mem_s=(shm_stats *)shmat(shm_des_s,NULL,0))==(shm_stats *)-1)
        error("shmat stats");
    mem_c->terminated=false;
 if(fork()==0){
        printf("Entra nella fork del processo AL\n");
        letter(mem_c,mem_s,sem_des,ALP);
    return 0;
    }

    if(fork()==0){
        printf("Entra nella fork del processo MZ\n");
        letter(mem_c,mem_s,sem_des,MZP);
    return 0;    
    }
    for(int i=0;i<strlen(mapped);i++){
        printf("Entra nel ciclo per acquisire i caratteri del file mappato in memoria\n");
       
        WAIT(sem_des,P);
        mem_c->c=tolower(mapped[i]);
        if(!isspace(mem_c->c) && !isdigit(mem_c->c)){
            if(mem_c->c>='a'&& mem_c->c<='l'){
                printf("Sveglia il processo AL\n");
                SIGNAL(sem_des,AL);
            }
            else{
                printf("Sveglia il processo MZ\n");
                SIGNAL(sem_des,MZ);
            }
        }

    }

    mem_c->terminated=true;


    
   

    //Stampa a video le statistiche 
    for(int i=0;i<26;i++)
        printf("%ld\n",mem_s->stats[i]);
    
   


    wait(NULL);
    wait(NULL);

    shmctl(shm_des_c,IPC_RMID,NULL);
    shmctl(shm_des_s,IPC_RMID,NULL);
    semctl(sem_des,0,IPC_RMID,NULL);
    munmap(mapped,sizeof(mapped));
    close(f_des);

    return 0;





}