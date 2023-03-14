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
#define MAX_BUF 20
// struttura per memoria condivisa
typedef struct auction
{
    char description[MAX_BUF];
    
} Auction;

// funzione di verifica esistenza e validità del file
bool doesFileExist(char **filename)
{
    struct stat filestat;
    if (stat(filename, &filestat) || !S_ISREG(filestat.st_mode))
        return false;
    return true;
}

// funzione di errore a runtime
void error(char **what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

int main(int argc, char **argv)
{
    // inizializzo sequenza pseudocasuale
    srand(time(NULL));

    // verifico correttezza chiamata di programma
    if (argc < 3 || (isdigit(argv[2]) == 0))
        error("uso: auction-house <auction-file> <num-bidders>");
    // verifico esistenza e validità di eventuali file
    if (!doesFileExist(argv[1]))
        error("il file selezionato non esiste o non è un file valido");

    // creo semafori e strutture ipc
    // ...

    // creo processo J
    if (fork() == 0){
        judge();
    }

    // creo processi B*
    for (int i = 0; i < argv[2]; i++)
    {
        if (fork() == 0)
        {
        }
    }
}