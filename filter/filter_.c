#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define MAX_BUF 1024
#define MAX_BUF_WORD 20
#define P 0

typedef struct {
    char line[MAX_BUF];
    bool eof;

}shm_data;


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

int init_shm(){
    int shm_des;
    if((shm_des=shmget(IPC_PRIVATE,sizeof(shm_data),IPC_CREAT | 0660))==-1)
        error("shmget");
    return shm_des;
}

int init_sem(int child_count){
    int sem_des;
    if((sem_des=semget(IPC_PRIVATE,child_count,IPC_CREAT | 0660))==-1)
        error("semget");
    if(semctl(sem_des,P,SETVAL,0)==-1)
        error("semctl P");
        for(int i=0;i<child_count-1;i++)
            if(semctl(sem_des,i+1,SETVAL,0)==-1)
                error("semctl F");
    
    return sem_des;
}

shm_data *get_shm(int shm_des){
    shm_data *mem;
    if((mem=(shm_data *)shmat(shm_des,NULL,0))==(shm_data *)-1)
        error("shmat");
    return mem;
}

int indexOf(char *str,char *substr){
    int counter=0;
    int index;
    for(int i=0;str[i]!='\0';i++){
        for(int j=0;substr[j]!='\0';j++){
            if(str[i+j]==substr[j]){
                if(counter==0)
                    index=i;
                counter++;
            }
            if(counter==strlen(substr))
                return i;

        }
       counter=0; 
    }
    return -1;
}

bool does_file_exist(char *path){
    struct stat s;
    if(stat(path,&s)==-1 || !S_ISREG(s.st_mode))
        return false;
    return true;
}

char* replaceWord(const char* s, const char* oldW,
                const char* newW)
{
    char* result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);
 
    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
 
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }
 
    // Making new string of enough length
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);
 
    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }
 
    result[i] = '\0';
    return result;
}

void filter(shm_data *mem,int sem_des,char *word, int id, int last){
    char type=word[0];
    char *found=NULL;
    char w[MAX_BUF_WORD];
    strncpy(w,word,MAX_BUF_WORD);
    memmove(w,w+1,strlen(w));
    
    if(type=='^'){
        while(1){
            
            if(mem->eof)
                {
                    if(id == last)
                        SIGNAL(sem_des,P);
                    else
                        SIGNAL(sem_des,id+1);
                    break;
                }
            WAIT(sem_des,id);
            if((found=strstr(mem->line,w))!=NULL){//Se ho trovato la sottostringa
                for(int i=0;i<strlen(w);i++)
                    mem->line[i+indexOf(mem->line,w)]=toupper(mem->line[i+indexOf(mem->line,w)]);
                //printf("Linea modificata: %s\n",mem->line);
            }
            if(id == last)
                    SIGNAL(sem_des,P);
                else
                    SIGNAL(sem_des,id+1);
        }
    }
    if(type=='%'){
        while(1){
            
            if(mem->eof)
                {
                    if(id == last)
                        SIGNAL(sem_des,P);
                    else
                        SIGNAL(sem_des,id+1);
                    break;
                }
            WAIT(sem_des,id);
            char *p1;
            char *p2;
            p1 = strtok(w,",");
            p2 = strtok(NULL,"\0");
            strcpy(mem->line,replaceWord(mem->line,p1,p2));
            if(id == last)
                    SIGNAL(sem_des,P);
                else
                    SIGNAL(sem_des,id+1);
        }
    }
}

int main(int argc,char *argv[]){
    if(argc<3)
        error("uso <file.txt> <filter1> [filter2]... [filtern]" );
    if(!does_file_exist(argv[1]))
        error("Il file non esiste");
    
    int shm_des=init_shm();
    shm_data *mem=get_shm(shm_des);
    int sem_des=init_sem(argc-1);

    FILE *f;
    if((f=fopen(argv[1],"r"))==NULL)
        error("Il file non è stato aperto con successo");
    char line[MAX_BUF];
    mem->eof=false;

    for(int i=0;i<argc-2;i++){
        if(fork()==0){//Processi F
            filter(mem,sem_des,argv[i+2],i+1,argc-2);
            return 0;
        }
    }

     while(fgets(line,MAX_BUF,f)){
        strncpy(mem->line,line,MAX_BUF);
        SIGNAL(sem_des,1);
        WAIT(sem_des,P);
        printf("%s\n",mem->line);
    }
    mem->eof=true;
    
    for(int i=0;i<argc-2;i++)
        SIGNAL(sem_des,1);//Sveglia tutti i figli per comunicare la fine della lettura dal file

    for(int i=0;i<argc-2;i++)
        wait(NULL);

    shmctl(shm_des,IPC_RMID,NULL);
    semctl(sem_des,P,IPC_RMID,0);
    semctl(sem_des,1,IPC_RMID,0);
    
}