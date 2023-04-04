#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>
#define R 3
#define C 5
#define MC 1
#define MN 2
#define MAX_BUFFER 32

typedef struct{
    long type;
    int card[R][C];
}msg_card;

typedef struct{
    long type;
    char buffer[MAX_BUFFER];
}msg_notify;

void error(char *what){
    fprintf(stderr,"%s,\n",what);
    exit(1);
}

int init_msg(){
    int msg_des;
    if((msg_des=msgget(IPC_PRIVATE,IPC_CREAT | 0660))==-1)
        error("msgget");
    return msg_des;
}

void create_card(msg_card *card_table){
    bool num[75]={false};
    int r;
    bool ok=false;
    for(int row=0;row<R;row++)
        for(int col=0;col<C;col++){
            ok=false;
            while(!ok){
            r=rand()%75;
                if(!num[r]){
                num[r]=true;
                card_table->card[row][col]=r+1;
                ok=true;
            }
        }
    }
}

void player(int id,int msg_des,int m){
    msg_card rcard;
    int tcard[R*m][C];
    for(int i=0;i<m;i++){
        msgrcv(msg_des,&rcard,sizeof(msg_card)-sizeof(long),MC,0);
        for(int row=i*R;row<R;row++)
            for(int col=0;col<C;col++){
                tcard[row][col]=rcard.card[row][col];
            }
    }

    for(int i=0;i<R*m;i++){
        for(int j=0;j<C;j++)
            printf("%d ",tcard[i][j]);
        printf("\n");
    }
    
}

int main(int argc,char *argv[]){
    if(argc<3)
        error("Uso <n> <m>");
    int n=atoi(argv[1]);
    int m=atoi(argv[2]);
    int msg_des=init_msg();
    
    for(int i=0;i<n;i++){
        if(fork()==0){//Processi Player
            player(i+1,msg_des,m);
            return 0;
        }
    }

    msg_card scard; 
    for(int i=0;i<n;i++){
        for(int j=0;j<m;j++){
        scard.type=MC;
        create_card(&scard);
        printf("invio card\n");
        msgsnd(msg_des,&scard,sizeof(msg_card)-sizeof(long),0);
        printf("card inviata\n");
        }
    }

    for(int i=0;i<n;i++)
        wait(NULL);
    
    msgctl(msg_des,IPC_RMID,NULL);
}