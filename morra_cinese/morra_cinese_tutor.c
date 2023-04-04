#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <time.h>
#define S_P1 0
#define S_P2 1
#define S_J 2
#define S_S 3
enum
{
    C,
    F,
    S
};
char *results[3] = {"carta", "forbici", "sasso"};

typedef struct
{
    char mossa_p1;
    char mossa_p2;
    char vincitore;
    bool done;

} shm_data;

int init_shm()
{
    int shm_des;
    if ((shm_des = shmget(IPC_PRIVATE, sizeof(shm_data), IPC_CREAT | 0660)) == -1)
    {
        perror("shmget");
        exit(1);
    }

    return shm_des;
}

int init_sem()
{
    int sem_des;
    if ((sem_des = semget(IPC_PRIVATE, 4, IPC_CREAT | 0660)) == -1)
    {
        perror("semget");
        exit(1);
    }

    if (semctl(sem_des, S_P1, SETVAL, 1) == -1)
    {
        perror("semget");
        exit(1);
    }
    if (semctl(sem_des, S_P2, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (semctl(sem_des, S_J, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (semctl(sem_des, S_S, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    return sem_des;
}
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

int whowins(char mossa1, char mossa2)
{
    if (mossa1 == mossa2)
        return 0;
    if ((mossa1 == C && mossa2 == S) || (mossa1 == F && mossa2 == C) || (mossa1 == S && mossa2 == F))
    {
        return 1;
    }
    return 2;
}

void player(char id, int shm_des, int sem_des)
{
    if (id == S_P1)
        srand(time(NULL));
    else
        srand(time(NULL) + 1);

    shm_data *data;
    if ((data = (shm_data *)shmat(shm_des, NULL, 0)) == (shm_data *)-1)
    {
        perror("shmat");
        exit(1);
    }

    while (1)
    {
        if (id == S_P1)
        {
            WAIT(sem_des, S_P1);

            if (data->done) // Esco quando il tabellone indica che sono finite le partite
                exit(0);
            data->mossa_p1 = rand() % 3;
            printf("P1 mossa %s\n", results[data->mossa_p1]);
        }
        else
        {
            WAIT(sem_des, S_P2);

            if (data->done)
                exit(0);
            data->mossa_p2 = rand() % 3;
            printf("P2 mossa %s\n", results[data->mossa_p2]);
        }
        SIGNAL(sem_des, S_J); // Sveglio il giudice
    }
}
void judge(int shm_des, int sem_des, unsigned n)
{
    shm_data *data;
    int winner;
    unsigned partite_completate = 0;
    if ((data = (shm_data *)shmat(shm_des, NULL, 0)) == (shm_data *)-1)
    {
        perror("shmat");
        exit(1);
    }

    while (1)
    {
        WAIT(sem_des, S_J); // Il giudice comincia a operare quando entrambi i giocatori hanno giocato
        WAIT(sem_des, S_J);
        winner = whowins(data->mossa_p1, data->mossa_p2);

        if (!winner)
        { // Se la partita è patta
            if (partite_completate == 0)
            {
                printf("J partita n.%d patta e quindi da ripetere\n", 1);
                SIGNAL(sem_des, S_P1); // Sveglio i giocatori per la prossima partita
                SIGNAL(sem_des, S_P2);
                continue;
            }
            else
            {

                printf("J partita n.%d patta e quindi da ripetere\n", partite_completate);
                SIGNAL(sem_des, S_P1); // Sveglio i giocatori per la prossima partita
                SIGNAL(sem_des, S_P2);
                continue;
            }
        }
        // Se qualcuno vince
        data->vincitore = winner;
        ++partite_completate;
        printf("J partita n.%d vinta da %d\n", partite_completate, winner);
        SIGNAL(sem_des, S_S); // Sveglio il tabellone

        if (partite_completate == n)
            break;
    }
    exit(0);
}
void scoreboard(int shm_des, int sem_des, unsigned n)
{ // La n indica il numero di partite da giocare
    shm_data *data;
    unsigned score[2] = {0, 0};

    if ((data = (shm_data *)shmat(shm_des, NULL, 0)) == (shm_data *)-1)
    {
        perror("shmat");
        exit(1);
    }
    for (int i = 0; i < n - 1; i++)
    {
        WAIT(sem_des, S_S);
        score[data->vincitore - 1]++; // In questo campo il processo giudice ha settato il vincitore della partita
        printf("T Classifica temporanea P1 %d P2 %d\n", score[0], score[1]);

        SIGNAL(sem_des, S_P1);
        SIGNAL(sem_des, S_P2);
    }
    WAIT(sem_des, S_S); // Cosa avviene per l'ultima partita
    score[data->vincitore - 1]++;
    data->done = 1; // Comunico la fine del torneo
    if (score[0] > score[1])
        printf("T Vincitore del torneo P1\n");
    else
        printf("T Vincitore del torneo P2\n");
    SIGNAL(sem_des, S_P1); // Sveglio i giocatori, che a loro volta termineranno il loro processo quando il campo done è settato a true
    SIGNAL(sem_des, S_P2);
    exit(0);
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <num-partite>", argv[0]);
        exit(1);
    }

    int shm_des = init_shm();
    int sem_des = init_sem();

    if (fork() == 0)
    {
        player(S_P1, shm_des, sem_des);
    }
    if (fork() == 0)
    {
        player(S_P2, shm_des, sem_des);
    }
    if (fork() == 0)
    {
        judge(shm_des, sem_des, atoi(argv[1]));
    }
    if (fork() == 0)
    {
        scoreboard(shm_des, sem_des, atoi(argv[1]));
    }

    for (int i = 0; i < 4; i++)
        wait(NULL);

    shmctl(shm_des, IPC_RMID, NULL);
    semctl(sem_des, 0, IPC_RMID);
    return 0;
}