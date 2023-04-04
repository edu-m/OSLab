#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#define MAX_BUF 25
#define MAX_PROC 32
enum SEMAPHORES{IN,DB,OUT};

typedef struct{
    int id;
    char query[MAX_BUF];
    int terminated;

}shm_dbin;

typedef struct{
    int id;
    char name[MAX_BUF];
    int value;

}shm_dbout;

typedef struct node{
    char name[MAX_BUF];
    int value;
    struct node *next;
}Node;

int WAIT(int sem_des, int num_semaforo){
struct sembuf operazioni[1] = {{num_semaforo,-1,0}};
return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo){
struct sembuf operazioni[1] = {{num_semaforo,+1,0}};
return semop(sem_des, operazioni, 1);
}

void insert(Node **testa,char *name,int value){
    Node *newNode = (Node *)(malloc(sizeof(Node)));
    // Nodo *nuovoNodo =  (Nodo *)(malloc(sizeof(Nodo)));
    strcpy(newNode->name,name);
    newNode->value=value;
    newNode->next=NULL;
    if(*testa==NULL)
        *testa=newNode;
    else{
        newNode->next=*testa;
        *testa=newNode;
    }
}

Node *search(Node *testa,char *name){
    Node *temp;
    //printf("%d\n", (testa == NULL));
    for(temp = testa; temp!=NULL; temp=temp->next){
        {
            //printf("sto comparando %s con %s, indovina che succede\n", temp->name, name);
            if(strncmp(temp->name,name,MAX_BUF)==0)
            return temp;}
    }
    return NULL;
}

void printList(Node *testa){
    printf("Sto cominciando il print della lista\n");
    for(Node *temp = testa; temp!=NULL; temp=temp->next)
        printf("Sono un Node di nome %s e valore %d\n",temp->name,temp->value);
}

void delete(Node *testa){
    Node *temp;
    while(testa!=NULL){
        temp=testa;
        testa=testa->next;
        free(temp);
    }
}

char *trim(char *name){
    char *trimmed = (char*)malloc(MAX_BUF);
    int c=0;
    for(int i=0;i<strlen(name);i++){
        if(!isspace(name[i]))
            trimmed[c++]=name[i];
    }
    trimmed[c+1]='\0';
    return trimmed;
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
    if(strcmp(type,"shm_dbin")==0){
        if((shm_des=shmget(IPC_PRIVATE,sizeof(shm_dbin),IPC_CREAT | 0660))==-1)
            error("shmget dbin");
    }
    else{
         if((shm_des=shmget(IPC_PRIVATE,sizeof(shm_dbout),IPC_CREAT | 0660))==-1)
            error("shmget dbout");

    }
    return shm_des;

}

int init_sem(){
    int sem_des;
    if((sem_des=semget(IPC_PRIVATE,3,IPC_CREAT | 0660))==-1)
        error("semget");
    if(semctl(sem_des,IN,SETVAL,1))
        error("semctl in");
    if(semctl(sem_des,DB,SETVAL,0))
        error("semctl db");
    if(semctl(sem_des,OUT,SETVAL,0))
        error("semctl out");
    return sem_des;

}

void in(char *path,int sem_des,int shm_des_dbin,int id){
    //Leggere dal rispettivo file
    FILE *f;
    shm_dbin *mem_dbin;
    if((f=fopen(path,"r"))==NULL){
        fprintf(stderr,"Errore nell'apertura del file da parte del processo %d\n",id);
        exit(1);
    }
    if((mem_dbin=shmat(shm_des_dbin,NULL,0))==(shm_dbin *)-1)
        error("shmat dbin");

    mem_dbin->terminated=false;    
    char line[MAX_BUF];
    char *trimmed;
    int counter=0;
    //char query[MAX_BUF];
    while(1){
        if(fgets(line,MAX_BUF,f) == NULL)
            {mem_dbin->terminated++;break;}
        trimmed=trim(line);
        WAIT(sem_des,IN);
        strncpy(mem_dbin->query,trimmed,MAX_BUF);
        mem_dbin->id=id;
        printf("IN%d inviata query n.%d '%s'\n",id+1,++counter,mem_dbin->query);
        SIGNAL(sem_des,DB);
    }
   
    fclose(f);
}

void db(char *path,int shm_des_dbin,int shm_des_dbout,int sem_des, Node *testa, Node *found, int num){
    //Lettura dal file e salvataggio in una lista
    FILE *f;
    shm_dbin *mem_dbin;
    shm_dbout *mem_dbout;
    
    char line[MAX_BUF];
    char *name;
    char *value;
    //int value;
    int tc=0;
    if((f=fopen(path,"r"))==NULL)
        error("Errore nell'apertura del file da parte del processo DB");
    if((mem_dbin=shmat(shm_des_dbin,NULL,0))==(shm_dbin *)-1)
        error("shmat dbin nel processo DB");
    if((mem_dbout=shmat(shm_des_dbout,NULL,0))==(shm_dbout *)-1)
        error("shmat dbout nel processo DB");
    
    while(fgets(line,MAX_BUF,f)){
        //Acquisisco nome e valore
        name=strtok(line,":");
        value=strtok(NULL,"\n");
        //Salvo nome e valore nella mia lista
        insert(&testa,name,atoi(value));
        // printList(testa);
    }
    //Valuto la query ricevuta
    while(1){
        WAIT(sem_des,DB);
   
    found=search(testa,mem_dbin->query);
    if(found==NULL){
        printf("DB Query '%s' da IN%d non trovata\n",mem_dbin->query,mem_dbin->id+1);
        SIGNAL(sem_des,IN);
    }
    else{
        printf("DB Query '%s' da IN%d  trovata con valore '%d'\n",found->name,mem_dbin->id+1,found->value);
        //Invio alla memoria condivisa con il processo OUT
        mem_dbout->id=mem_dbin->id;
        strncpy(mem_dbout->name,found->name,MAX_BUF);
        mem_dbout->value=found->value;
        SIGNAL(sem_des,OUT);
        SIGNAL(sem_des,IN);
        }
            if(mem_dbin->terminated == num)
        break;
    }

    fclose(f);
}

void out(int shm_des_dbout,int shm_des_dbin,int sem_des,int num){
    int counter[MAX_PROC] = {0};
    int results[MAX_PROC] = {0};
    shm_dbin *mem_dbin;
    shm_dbout *mem_dbout;
    int c = 0;
    if((mem_dbin=shmat(shm_des_dbin,NULL,0))==(shm_dbin *)-1)
        error("shmat dbin nel processo OUT");
    if((mem_dbout=shmat(shm_des_dbout,NULL,0))==(shm_dbout *)-1)
        error("shmat dbout nel processo OUT");
    while(1){
        WAIT(sem_des,OUT);//Procedo se vengo svegliato dal processo DB
        counter[mem_dbout->id]++;
        results[mem_dbout->id]+=mem_dbout->value;
        if(mem_dbin->terminated == num)
            break;
    }
    for(int i=0;i<num;i++)
        printf("numero di match per IN%d: %d con somma = %d\n",i+1,counter[i],results[i]);
}

int main(int argc,char *argv[]){
    Node *testa=NULL;
    Node *found=NULL;
    if(argc<3)
        error("Uso <db.txt> <query_1.txt> ... <query_n.txt>");
    for(int i=0;i<argc-1;i++){
        if(!does_file_exists(argv[i+1]))
            error("Non tutti i file sono regolari");
    }
   
    int shm_des_dbin=init_shm("shm_dbin");
    int shm_des_dbout=init_shm("shm_dbout");
    int sem_des=init_sem();

    if(fork()==0){//Processo db 
        db(argv[1],shm_des_dbin,shm_des_dbout,sem_des, testa, found, argc-2);
        return 0;
    }
    
   for(int i=0;i<argc-2;i++)//Processi in
        if(fork()==0)
        {
            in(argv[i+2],sem_des,shm_des_dbin,i);
            return 0;
        }

   if(fork()==0){//Processo out
       out(shm_des_dbout,shm_des_dbin,sem_des,argc-2);
       return 0;
   }

    for(int i=0;i<argc-1;i++){
        wait(NULL);
    }

    shmctl(shm_des_dbin,IPC_RMID,NULL);
    shmctl(shm_des_dbout,IPC_RMID,NULL);
    semctl(sem_des,IN,IPC_RMID,NULL);
    semctl(sem_des,DB,IPC_RMID,NULL);
    semctl(sem_des,OUT,IPC_RMID,NULL);
    delete(testa);
}
