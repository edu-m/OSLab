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
#define CINQUINA 0
#define BINGO 1

typedef struct{
    long type;
    int card[R][C];
}msg_card;

typedef struct{
    long type;
    int value;
}msg_notify;

bool checkCinquina(int card[][C], bool *num)
{
    int cont;
    for(int i=0;i<R;i++)
    {
        cont = 0;
        for(int j=0;j<C;j++)    
        {
            if(num[card[i][j]-1] == false)
                {i++;j=0;}
            else ++cont;
        }
        if(cont == 5)
        return true;
    }
}

bool checkBingo(int card[][C], bool *num){
    for(int i=0;i<R;i++)
        for(int j=0;j<C;j++)
            if(num[card[i][j]-1] == false)
                return false;
    return true;
}

int checkPlayerCinquina(int card[][C], bool *num, int m)
{
    int cont;
    for(int i=0;i<R*m;i++)
    {
        cont = 0;
        for(int j=0;j<C;j++)    
        {
            if(num[card[i][j]-1] == false)
                {i++;j=0;}
            else ++cont;
        }
        if(cont == 5)
        return i;
    }
    return -1;
}

bool checkPlayerBingo(int card[][C], bool *num, int m){
    int cont;
    for(int i=0;i<m;i++)
    {
        cont = 0;
        for(int j=0;j<R;j++)
            for(int k=0;k<C;k++)
                if(num[card[j+(i*R)][k]-1] == false)
                    {i++;j=0;k=0;}
                else 
                    cont++;
    }
    return true;
}


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
    bool num[75] = {false};
    msg_card rcard;
    msg_notify ecard;
    msg_card snt_card;
    snt_card.type;
    int tcard[R*m][C];
    for(int i=0;i<m;i++){
        msgrcv(msg_des,&rcard,sizeof(msg_card)-sizeof(long),MC,0);
        for(int row=0;row<R;row++){ // i*R
            for(int col=0;col<C;col++){
                tcard[row+(i*R)][col]=rcard.card[row][col];
            }
        }
    }
    while(1){
        msgrcv(msg_des,&ecard,sizeof(msg_notify)-sizeof(long),MN,0);
        if(ecard.value == -1)
            break;
        num[ecard.value-1] = true;

        if(checkPlayerCinquina(tcard,num,m) != -1){
            snt_card.type = CINQUINA;
            //snt_card.card;
            //msgsnd();
        }
    }
}

int extract(bool *num){
    int generated;
    while(1){
        generated = rand() % 75;
        if(!num[generated])
        {
            num[generated] = true;
            return generated+1;
        }
    }
}

bool nums_left(bool *num){
    for(int i=0;i<75;i++)
        if(num[i] == false) 
        return true;
    return false;
}



int main(int argc,char *argv[]){
    srand(time(NULL));
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
    msg_notify extraction;
    msg_card rcvd_card;
    for(int i=0;i<n;i++){
        for(int j=0;j<m;j++){
        scard.type=MC;
        create_card(&scard);
        msgsnd(msg_des,&scard,sizeof(msg_card)-sizeof(long),0);
        }
    }
    bool cinquina = false;
    bool bingo = false;
    bool num[75];
    extraction.type = MN;
    while(!bingo){ // "main" del gioco
        if(nums_left(num))
        {
            extraction.value = extract(num);
            for(int i=0;i<n;i++)
            msgsnd(msg_des,&extraction,sizeof(msg_notify)-sizeof(long),0);
        }
        else
        {
            extraction.value = -1;
            for(int i=0;i<n;i++)
            msgsnd(msg_des,&extraction,sizeof(msg_notify)-sizeof(long),0);
            break;
        }
        //for(int i=0;i<n;i++){
        //    msgrcv(msg_des,&rcvd_card,sizeof(msg_card)-sizeof(long),MC,IPC_NOWAIT);
        //    if(!cinquina && rcvd_card.type == CINQUINA && checkCinquina(rcvd_card.card, num))
        //        cinquina = true;
        //    if(!bingo && rcvd_card.type == BINGO && checkBingo(rcvd_card.card, num))
        //        bingo = true;
        //}
    }

    for(int i=0;i<n;i++)
        wait(NULL);
    
    msgctl(msg_des,IPC_RMID,NULL);
}

