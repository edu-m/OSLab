#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <semaphore.h>
#define P 0
#define G 1
#define T 2

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

typedef struct morra_cinese
{
    char mosse[2];
    int vincitore;
    bool patta;
    int games;
    bool terminato;
} Gioco;

int get_vincitore(char g1, char g2)
{
    if (g1 == g2)
        return -1;
    if (g1 == 'S')
    {
        if (g2 == 'C')
            return 1;
        else
            return 0;
    }
    else if (g1 == 'C')
    {
        if (g2 == 'S')
            return 0;
        else
            return 1;
    }
    else
    {
        if (g2 == 'S')
            return 1;
        else
            return 0;
    }
}

char get_random_move()
{

    char letters[3] = {'S', 'C', 'F'};
    return letters[rand() % 3];
}

void error(char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

int get_shm_id()
{
    int shm_id;
    if ((shm_id = shmget(IPC_PRIVATE, sizeof(Gioco), IPC_CREAT | 0660)) == -1)
        error("in creazione memoria condivisa");
}

Gioco *get_shm(int id)
{
    Gioco *gioco;
    if ((gioco = (Gioco *)shmat(id, NULL, 0)) == MAP_FAILED)
        error("in attach alla memoria condivisa");
    return gioco;
}

int get_sem_id()
{
    int sem_id;
    if ((sem_id = semget(IPC_PRIVATE, 3, IPC_CREAT | 0660)) == -1)
        error("in creazione semaforo");
    if (semctl(sem_id, P, SETVAL, 2) == -1)
        error("In assegnazione di valore a semaforo player");
    if (semctl(sem_id, G, SETVAL, 0) == -1)
        error("In assegnazione di valore a semaforo giudice");
    if (semctl(sem_id, T, SETVAL, 0) == -1)
        error("In assegnazione di valore a semaforo tabellone");
    return sem_id;
}

void giocatore(bool player_id, Gioco *gioco, int sem_id)
{
    srand(time(NULL) * player_id);
    char random_move;
    while (1)
    {
        if (gioco->terminato)
            break;
        WAIT(sem_id, P);
        random_move = get_random_move();
        printf("Mossa di %d: %c\n", player_id, random_move);
        gioco->mosse[player_id] = random_move;
        SIGNAL(sem_id, G);
        sleep(1);
    }
}

void giudice(Gioco *gioco, int sem_id)
{
    while (1)
    {
        sleep(1);
        if (gioco->terminato)
            break;
        WAIT(sem_id, G);
        WAIT(sem_id, G);
        int vincitore = get_vincitore(gioco->mosse[0], gioco->mosse[1]);
        gioco->vincitore = vincitore;
        if (vincitore == -1)
        {
            printf("Nessun vincitore\n");
            gioco->patta = true;
            SIGNAL(sem_id, P);
            SIGNAL(sem_id, P);
        }
        else
        {
            printf("Gioco terminato: %d\n", (gioco->terminato = (--gioco->games) == 0));
            gioco->patta = false;
            SIGNAL(sem_id, T);
        }
    }
}

void tabellone(Gioco *gioco, int sem_id)
{
    int vittorie[2] = {0, 0};
    while (!gioco->terminato)
    {
        sleep(1);
        WAIT(sem_id, T);
        ++vittorie[gioco->vincitore];
        printf("Vince %d!\n", gioco->vincitore);
        printf("Mossa di 0: %c\n", gioco->mosse[0]);
        printf("Mossa di 1: %c\n", gioco->mosse[1]);

        printf("Punteggi: G1 %d G2 %d\n", vittorie[0], vittorie[1]);
        if (gioco->games > 0)
        {
            printf("faccio signal su giocatore\n");
            /// SIGNAL(sem_id, P);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
        error("uso: morra-cinese <numero-partite>");
    if (atoi(argv[1]) < 1)
        error("Inserire un numero positivo e maggiore di zero");
    int shm_id = get_shm_id();
    Gioco *gioco = get_shm(shm_id);
    int sem_id = get_sem_id();
    gioco->games = atoi(argv[1]);
    gioco->terminato = false;

    /*Inizio gioco*/
    if (fork() == 0) // giocatore 1
    {
        giocatore(0, gioco, sem_id);
        return 0;
    }

    if (fork() == 0) // giocatore 2
    {
        giocatore(1, gioco, sem_id);
        return 0;
    }

    if (fork() == 0) // giudice
    {
        giudice(gioco, sem_id);
        return 0;
    }

    if (fork() == 0) // tabellone
    {
        tabellone(gioco, sem_id);
        return 0;
    }
    /*Fine gioco*/

    for (int i = 0; i < 4; i++)
        wait(NULL); // join

    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
}