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
#include <ctype.h>
#include <unistd.h>

#define LONG_BUFSIZE 26
#define AL 0
#define MZ 1
#define P 2

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

void error(const char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

bool fileExists(char *fileName, struct stat *fileStat)
{
    if (stat(fileName, fileStat) || !S_ISREG(fileStat->st_mode))
        return false;
    return true;
}

typedef struct charmem
{
    bool eof;
    char input;
} Charmem;

typedef struct longmem
{
    long input[LONG_BUFSIZE];
} Longmem;

void writeStats(int self_id, Charmem *cmem, Longmem *lmem, int idSem)
{
    while (!cmem->eof)
    {
        WAIT(idSem, self_id);
        if (isspace(cmem->input) == 0)
            lmem->input[(int)toupper(cmem->input) - 65]++; // g = 103, G = 71, 71-64 = 7-1 = 6;
        SIGNAL(idSem, P);
    }
}

void readFile(char *mfile, size_t size, struct charmem *mem, int idSem)
{
    char *line;
    for (size_t i = 0; i < size; i++)
    {
        mem->input = tolower(mfile[i]);
        if ((int)(toupper(mem->input) - 64) > 12)
        {
            // printf("signal su MZ: rilevata %c\n", mfile[i]);
            SIGNAL(idSem, MZ);
        }
        else
        {
            // printf("signal su AL: rilevata %c\n", mfile[i]);
            SIGNAL(idSem, AL);
        }
        WAIT(idSem, P);
    }
    mem->eof = true;
}

int main(int argc, char **argv)
{
    Charmem *charmem;
    Longmem *longmem;
    struct stat fileStat;
    // soliti check di routine
    if (argc < 2)
        error("use alpha-stats <file.txt>");
    if (!fileExists(argv[1], &fileStat))
        error("while opening");

    // creazione memoria condivisa
    int s_charm = shmget(IPC_PRIVATE, sizeof(Charmem), IPC_CREAT | 0660);
    int s_longm = shmget(IPC_PRIVATE, sizeof(Longmem), IPC_CREAT | 0660);
    if (s_charm == -1 || s_longm == -1)
        error("while creating shared mem");

    charmem = (Charmem *)shmat(s_charm, NULL, 0);
    longmem = (Longmem *)shmat(s_longm, NULL, 0);
    charmem->eof = false;

    // inizializza memoria condivisa a 0
    for (int i = 0; i < LONG_BUFSIZE; i++)
        longmem->input[i] = 0;

    if (charmem == MAP_FAILED || longmem == MAP_FAILED)
        error("while accessing shared mem");

    // creazione semaforo
    int idSem = semget(IPC_PRIVATE, 3, IPC_CREAT | 0660);
    if (idSem == -1)
        error("while creating semaphore");
    for (int i = 0; i < 3; i++)
        semctl(idSem, i, SETVAL, 0);

    // mappatura file su memoria
    int file = open(argv[1], O_RDONLY);
    if (file == -1)
        error("while opening file");
    char *mfile = (char *)mmap(NULL, fileStat.st_size, PROT_READ, MAP_SHARED, file, 0);
    if (mfile == MAP_FAILED)
        error("while mapping file");

    // istanziamo processo MZ
    if (fork() == 0)
    {
        writeStats(MZ, charmem, longmem, idSem);
        return 0;
    }

    // istanziamo processo AL
    if (fork() == 0)
    {
        writeStats(AL, charmem, longmem, idSem);
        return 0;
    }

    readFile(mfile, fileStat.st_size, charmem, idSem);

    for (int i = 0; i < LONG_BUFSIZE; i++)
        printf("%c: %ld\n", (char)(65 + i), longmem->input[i]);

    munmap(file, fileStat.st_size);

    shmctl(s_charm, IPC_RMID, NULL);
    shmctl(s_longm, IPC_RMID, NULL);

    semctl(idSem, P, IPC_RMID);
    semctl(idSem, AL, IPC_RMID);
    semctl(idSem, MZ, IPC_RMID);
}