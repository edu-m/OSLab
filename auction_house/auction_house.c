#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sem.h>
#define B 1
#define J 2
#define MAX_BIDDERS 8

#define MAX_BUF 32

typedef struct{
    long type;
    char buffer[MAX_BUF];

}msg_data;

msg_data *creat_msg(char *buffer,long type){
    msg_data *msg=(msg_data *)(malloc(sizeof(msg_data)));
    msg->type = type;
    strncpy(msg->buffer,buffer,MAX_BUF);
    //printf("Stampa il messaggio %ld %s\n",msg->type,msg->buffer);
    return msg;
}

void error(char *what){
    fprintf(stderr,"%s\n",what);
    exit(1);
}

bool does_file_exists(char * path){
    struct stat s;
    if(stat(path,&s)==-1 || !S_ISREG(s.st_mode))
        return false;
    return true;
}

int init_msg(){
    int msg_des;
    if((msg_des=msgget(IPC_PRIVATE,IPC_CREAT | 0660))==-1)
        error("msgget");
    return msg_des;
}

int get_winner(int *offers,int min_offer,int num_bidders){
    int max=min_offer;
    int r=-1;
    for(int i=0;i<num_bidders;i++){
        if(offers[i]>=max){
            max=offers[i];
            r=i;
            }
    }
    return r;
}


void bidder(int msg_des,int id){
    srand(time(NULL)+id*2);
    msg_data received;
    msg_data *sent;
    char buffer[MAX_BUF];
    int offer;
    int max_offer;
    int sleep_time;
    while(1){
        if(msgrcv(msg_des,&received,MAX_BUF,B,0)==-1)
            error("msgrcv bidder");
        if(strncmp(received.buffer,"<terminated>",MAX_BUF)==0)
            break;
        strtok(received.buffer,",");
        strtok(NULL,",");
        max_offer=atoi(strtok(NULL,"\n"));
        offer=rand()%max_offer+1;
        sprintf(buffer,"%d,%d\n",id,offer);
        printf("Invio messaggio di tipo J\n");
        sent=creat_msg(buffer,J);
        printf("Messaggio inviato dal B%d %s\n",id+1,sent->buffer);
        sleep_time = rand()%4;
        printf("B%d: aspetto per %d secondi\n",id+1,sleep_time);
        sleep(sleep_time);
        if(msgsnd(msg_des,sent,strlen(sent->buffer)+1,0)==-1)
            perror("msgsnd per il processo J");
    }
}

int main(int argc,char *argv[]){
    if(argc<3)
        error("Uso <file.txt> <num_bidders>");
    if(!does_file_exists(argv[1]))
        error("Il file non esiste");
    int num_bidders=atoi(argv[2]);
    char *path=argv[1];

    int msg_des=init_msg();

    FILE *f;
    if((f=fopen(path,"r"))==NULL)
        error("fopen");
    char line[MAX_BUF];
    char l[MAX_BUF];
    int offers[MAX_BIDDERS] = {0};
    char *obj;
    int max_offer;
    int min_offer;
    msg_data *msg;
    msg_data receive;
    int ac=0;
    int id_bidder;
    int offer_bidder;
    int bids;

    for(int i=0;i<num_bidders;i++)
        if(fork()==0)
            {
                bidder(msg_des,i);
            return 0;}

    //Lettura del file riga per riga e invio del messaggio
    while(fgets(line,MAX_BUF,f)){
        bids = 0;
        ++ac;
        obj=strtok(line,",");
        min_offer=atoi(strtok(NULL,","));
        max_offer=atoi(strtok(NULL,"\n"));

        sprintf(l,"%d,%s,%d\n",ac,obj,max_offer);
        //Crea un messaggio destinato ai bidders
        msg=creat_msg(l,B); 
        for(int i=0;i<num_bidders;i++)
            if(msgsnd(msg_des,msg,strlen(msg->buffer)+1,0)==-1)
                perror("msgsnd nel ciclo");
        printf("J Lancio asta n.%d per %s con minimo %d EURO e massimo %d EURO\n",ac,obj,min_offer,max_offer);
        //Riceve i messaggi dai bidders, salva le loro offerte 
        
         while(bids < num_bidders){
            msgrcv(msg_des,&receive,MAX_BUF,J,0);
            ++bids;
            id_bidder=atoi(strtok(receive.buffer,","));
            offer_bidder=atoi(strtok(NULL,"\n"));
            printf("Il processo B%d offre %d EURO\n",id_bidder+1,offer_bidder);
            offers[id_bidder]=offer_bidder; 
         }
         //Decide il vincitore dell'asta
        //  printf("Tutte le offerte per l'asta %d sono state ricevute\n",ac);
         id_bidder=get_winner(offers,min_offer,num_bidders);
         if(id_bidder==-1)
            printf("J Asta n.%d andata a vuoto\n",ac);
        else
            printf("J Asta n.%d aggiudicata da B%d con %d EURO\n",ac,id_bidder+1,offers[id_bidder]);
    }
    msg=creat_msg("<terminated>",B);
    for(int i=0;i<num_bidders;i++)
        if(msgsnd(msg_des,msg,MAX_BUF,0)==-1)
                perror("msgsnd per indicare la fine del file");

    for(int i=0;i<num_bidders;i++){
       wait(NULL);
    }

    msgctl(msg_des,IPC_RMID,NULL);
    fclose(f);
}