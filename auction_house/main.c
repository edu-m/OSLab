#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#define MAX_BUF 50
#define NUM_SEM 2
#define DELIM ","
#define SEM_J 0
#define SEM_B 1

// struttura per memoria condivisa
typedef struct auction
{
    char description[MAX_BUF];
    int min_offer;
    int max_offer;
    int curr_offer;
    int id;
    int bids;
} Auction;

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

// funzione di verifica esistenza e validità del file
bool doesFileExist(char *filename)
{
    struct stat filestat;
    if (stat(filename, &filestat) || !S_ISREG(filestat.st_mode))
        return false;
    return true;
}

int getRandom(int lbound, int ubound)
{
    return rand() % ubound + lbound;
}

// funzione di errore a runtime
void error(char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

void judge(Auction *mem, int semId, char *filename, int bidders)
{

    FILE *file = fopen(filename, "r");
    char line[MAX_BUF];
    char *description;
    char *min_offer;
    char *max_offer;
    int i = 0;
    while (fgets(line, MAX_BUF, file))
    {
        mem->bids = 0;
        description = strtok(line, ",");
        min_offer = strtok(NULL, ",");
        max_offer = strtok(NULL, "\n");
        printf("J: lancio asta n.%d per %s con offerta minima di %d EUR e massima di %d EUR\n", i, description, atoi(min_offer), atoi(max_offer));
        WAIT(semId, SEM_J);
        strncpy(mem->description, description, strlen(description));
        mem->min_offer = atoi(min_offer);
        mem->max_offer = atoi(max_offer);
        SIGNAL(semId, SEM_B);
    }
}

void bidder(int id, Auction *mem, int idSem)
{
    WAIT(idSem, SEM_B);
    int randomBid = getRandom(mem->min_offer, mem->max_offer);
    if (mem->curr_offer < randomBid)
    {
        mem->curr_offer = randomBid;
        mem->id = id;
        ++mem->bids;
    }
    SIGNAL(idSem, SEM_B);
}

int main(int argc, char **argv)
{
    // inizializzo sequenza pseudocasuale
    srand(time(NULL));

    // verifico correttezza chiamata di programma
    if (argc < 3 || !isdigit(*argv[2]))
        error("uso: auction-house <auction-file> <num-bidders>");
    // verifico esistenza e validità di eventuali file
    if (!doesFileExist(argv[1]))
        error("il file selezionato non esiste o non è un file valido");
    // creo semafori e strutture ipc
    int idSem = semget(IPC_PRIVATE, 2, IPC_CREAT | 0660);
    semctl(idSem, 0, SETVAL, 1);
    semctl(idSem, 1, SETVAL, 0);
    int memseg_id = shmget(IPC_PRIVATE, sizeof(Auction), IPC_CREAT | 0660);
    if (memseg_id == -1)
        error("in allocazione memoria condivisa");
    Auction *auc;
    if ((auc = (Auction *)shmat(memseg_id, NULL, 0)) == MAP_FAILED)
        error("in attach alla memoria condivisa");
    auc->curr_offer = 0;

    // creo processo J
    if (fork() == 0)
    {
        judge(auc, idSem, argv[1], atoi(argv[2]));
    }

    // creo processi B*
    // for (int i = 0; i < argv[2]; i++)
    // {
    //     if (fork() == 0)
    //     {
    //         bidder(i, auc, idSem);
    //     }
    // }
}