#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#define MAX_BUF 32
#define P 0
#define W 1

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

typedef struct shm_mem{
    char buffer[MAX_BUF];
} Mem;

typedef struct msg{
    long msgtyp;
    char buffer[MAX_BUF];
} Message;

bool isPalindrome(char *word){
    //printf("%s ",word);
    int len = strlen(word);

    for (int i = 0; i < len/2; i++) {
        if (word[i] != word[len-1-i]) 
            return false;
    }
    return true;
}

char *trim(char *line)
{
    size_t i;
    size_t j=0;
    char *trimmed = (char *)(malloc(MAX_BUF));
    for (i = 0; i < strlen(line); i++)
    {
        if (!isspace(line[i]))
            trimmed[j++] = line[i];
    }
    trimmed[i + 1] = '\0';
    return trimmed;
}

void error(char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

bool doesFileExist(char *filename, struct stat* file)
{
    if (stat(filename, file) || !S_ISREG(file->st_mode))
        return false;
    return true;
}

void reader(char *filename, struct stat *file, int msg_q)
{
    Message *msg = (Message*)malloc(sizeof(Message));
    char *lines;
    int file_des = open(filename, O_RDONLY);
    lines = (char*)mmap(NULL,file->st_size,PROT_READ,MAP_SHARED,file_des,0);
    if (lines == MAP_FAILED)
        error("in mappatura file su memoria");
    char buffer[MAX_BUF];
    memset(buffer,0,MAX_BUF);
    char curr[2];
    for(int i=0;i<file->st_size;i++){
        if(lines[i] != '\n')
        {
            curr[0] = lines[i];
            curr[1] = '\0';
            strcat(buffer,curr);
        }
        else 
        {
            // inviare tramite coda di messaggi
            msg->msgtyp = 1;
            strcpy(msg->buffer,buffer);
            msgsnd(msg_q,msg,strlen(buffer)+1,0);
            memset(buffer,0,MAX_BUF);
        }
    }
    const char *term = "-finito-";
    msg->msgtyp = 1;
    memset(msg->buffer,0,sizeof(msg->buffer));
    strcpy(msg->buffer,term);
    msgsnd(msg_q,msg,strlen(term)+1,0);
}


void writer(Mem *mem, int sem_id)
{
    // usare memoria condivisa
    char line[MAX_BUF];
    while(1){
        WAIT(sem_id,W);
        // strcpy(line,mem->buffer);
        fflush(stdout);
        fprintf(stdout,"%s",mem->buffer);
        SIGNAL(sem_id,P);
        if(strcmp(line, "-finito-") == 0)
        {
            fflush(stdout);
            fprintf(stdout,"\n");
            break;
        }  
    }
}

int get_msgq(){
    int msg_q;
    if((msg_q = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1)
        error("in msgget");
    return msg_q;
}

Mem *get_mem(){
    int mem_id;
    Mem *mem;
    if((mem_id = shmget(IPC_PRIVATE,sizeof(Mem),IPC_CREAT | 0660)) == -1)
        error("in shmget");
    if ((mem = (Mem*)shmat(mem_id,NULL,0)) == MAP_FAILED)
        error("in shmat");
    return mem;
}

int get_sem(){
    int sem_id;
    if((sem_id = semget(IPC_PRIVATE,2,IPC_CREAT | 0660)) == -1)
        error("in msgget");
    if((semctl(sem_id,P,SETVAL,1)) == -1)
        error("in semctl P");
    if((semctl(sem_id,W,SETVAL,0)) == -1)
        error("in semctl W");
    return sem_id;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        error("uso: palindrome-filter <input file>");
    struct stat file;
    if (!doesFileExist(argv[1], &file))
        error("file non trovato o non accessibile");
    
    Mem *mem = get_mem();
    int sem_id = get_sem();
    int msg_q = get_msgq();
    
    if (fork() == 0)
    {
        reader(argv[1],&file, msg_q);
        return 0;
    }
    if (fork() == 0)
    {
        writer(mem,sem_id);
        return 0;
    }

    Message rcvd_msg;
    char *trimmedline;
    while(1){
        msgrcv(msg_q,&rcvd_msg,MAX_BUF,0,0);
        WAIT(sem_id,P);
        trimmedline = trim(rcvd_msg.buffer);
        memset(mem->buffer,0,sizeof(mem->buffer));
        if(isPalindrome(trimmedline))
            strcpy(mem->buffer,trimmedline);
        if(strcmp(trimmedline, "-finito-") == 0)
        {
            strcpy(mem->buffer,"-finito-");
            SIGNAL(sem_id,W);
            break;
        }
        SIGNAL(sem_id,W);
    }
    // cancellazione strutture ipc

    
} 
