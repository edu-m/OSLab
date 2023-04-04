/*Il processo padre crea tre pocessi figli, ovvero J (giudice), P1 (giocatore 1) e P2 (giocatore 2).
Si occupa poi di distruggere le strutture per la comunicazione dei processi, al termine delle 
partite.
J legge dal file riga per riga un intero e lo salva nella memoria condivisa. Dopo di che permette
ai processi P1 e P2 di generare un numero casuale, che salveranno in due altre porzioni
della stessa memoria condivisa. Se i giocatori generano lo stesso numero, il match si
ripete. Altrimenti il giocatore che si avvicina di più all'intero che sarà il target vincerà il
match. J stamperà a video, partita per partita, i vincitori. Le partite terminano quando termina il file da cui J legge. Utilizzare il numero sufficiente
di semafori*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <string.h>
#define MAX_BUF 32
enum{J,P};

typedef struct{
    int target;
    int mossa[2];
    int vincitore;
    bool finito;
    int id;
}shm_data;

typedef struct{
    long type;
    char buffer[MAX_BUF];

}msg_data;

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
    if(stat(path,&s)==1 || !S_ISREG(s.st_mode))
        return false;
    return true;
}

int init_sem(){
    int sem_des;
    if((sem_des=semget(IPC_PRIVATE,3,IPC_CREAT | 0660))==-1)
        error("semget");
    if(semctl(sem_des,J,SETVAL,2)==-1)
        error("semctl J");
    if(semctl(sem_des,P,SETVAL,0)==-1)
        error("semctl P");
    return sem_des;
}

int init_shm(){
    int shm_des;
    if((shm_des=shmget(IPC_PRIVATE,sizeof(shm_data),IPC_CREAT | 0660))==-1)
        error("shmget");
    return shm_des;

}

shm_data *get_shm(int shm_des){
    shm_data *data;
    if((data=(shm_data *)shmat(shm_des,NULL,0))==(shm_data *)-1)
        error("shmat");
    return data;

}

int init_msg(){
    int msg_des;
    if((msg_des=msgget(IPC_PRIVATE,IPC_CREAT | 0660))==-1)
        error("msgget");
    return msg_des;

}

void judge(int sem_des,int msg_des, shm_data *mem, char *path){
    FILE *f;
    char line[MAX_BUF];
    msg_data *msg=(msg_data *)(malloc(sizeof(msg_data)));
    msg->type=1;
    mem->finito=false;
    int i = 0;
    if((f=fopen(path,"r"))==NULL)
        error("Errore nell'apertura del file");
    while(!feof(f)){
        fgets(line,MAX_BUF,f);
        mem->target=atoi(line);
        WAIT(sem_des,J);
        WAIT(sem_des,J);
        SIGNAL(sem_des,P);
        SIGNAL(sem_des,P);
        usleep(75);
        if((mem->target-mem->mossa[0])<(mem->target-mem->mossa[1]))
            mem->vincitore=1;
        else 
            mem->vincitore=2;

        sprintf(msg->buffer,"%d,%d,%d,%d,%d",++i,mem->mossa[0],mem->mossa[1],mem->vincitore,mem->target);
            if(msgsnd(msg_des,msg,strlen(msg->buffer)+1,0)==-1)
                perror("msgsnd");
        
        memset(line,0,sizeof(line));
        }
        mem->finito = true;
}

void player(int sem_des, shm_data *mem, int id){
    srand(time(NULL)+id*2);
    int random;
    while(1){
        WAIT(sem_des,P);
        random = rand() % mem->target + 1;
        mem->mossa[id-1] = random;
        mem->id = id;
        SIGNAL(sem_des,J);
        if(mem->finito)
            break;
    }
}

void out(int msg_des,int sem_des,shm_data *mem){
    msg_data msg;
    int k;
    int v;
    int m1;
    int m2;
    int t;
    
    while(1){
        //sleep(1);
        if(mem->finito==true)
            break;
        if(msgrcv(msg_des,&msg,MAX_BUF,0,0)==-1)
            error("msgrcv");
        k = atoi(strtok(msg.buffer,","));
        m1=atoi(strtok(NULL,","));
        m2=atoi(strtok(NULL,","));
        v=atoi(strtok(NULL,","));
        t=atoi(strtok(NULL,"\n"));
        
    if(t != 0)
    {
        if(v==1)
             printf("%d: Vince P1 con %d su P2 con %d rispetto al target %d\n",k,m1,m2,t);
        else
             printf("%d: Vince P2 con %d su P1 con %d rispetto al target %d\n",k,m2,m1,t);}
        SIGNAL(sem_des,J);       
    }
}

int main(int argc, char *argv[]){
    if(argc<2)
        error("Uso <file.txt>");
    if(!does_file_exists(argv[1]))
        error("Il file non esiste");
    
    int sem_des=init_sem();
    int shm_des=init_shm();
    
    shm_data *mem=get_shm(shm_des);
    int msg_des=init_msg();
    

    if(fork()==0){//J
        judge(sem_des,msg_des,mem,argv[1]);
        return 0;
    }
    if(fork()==0){//P1
        player(sem_des,mem,1);
        return 0;
    }
    if(fork()==0){//P2
       player(sem_des,mem,2);  
       return 0;
    }
     if(fork()==0){//out
       out(msg_des,sem_des,mem);  
       return 0;
    }
    
    wait(NULL);
    wait(NULL);
    semctl(sem_des,0,IPC_RMID,NULL);
    shmctl(shm_des,IPC_RMID,NULL);
    msgctl(msg_des,IPC_RMID,NULL);
    
    return 0;
}