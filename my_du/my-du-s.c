#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#define STATER 0
#define SCANNER 1
#define MAX_BUF 512

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

typedef struct shm_path
{
    char pathname[MAX_BUF];
    int id;
    bool done;
} Shm_path;

typedef struct msg
{
    long msgtyp;
    char data[MAX_BUF];
} Message;

void error(char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

bool doesPathExist(char *pathname)
{
    struct stat path_stat;
    if (stat(pathname, &path_stat) || !S_ISDIR(path_stat.st_mode))
        return false;
    return true;
}

int get_shm_id()
{
    int shm_id;
    if ((shm_id = shmget(IPC_PRIVATE, sizeof(Shm_path), IPC_CREAT | 0660)) == -1)
        error("in shmget");
    return shm_id;
}

Shm_path *get_shm(int shm_id)
{
    Shm_path *shm_path;
    if ((shm_path = (Shm_path *)shmat(shm_id, NULL, 0)) == MAP_FAILED)
        error("in shmat");
}

int get_msg_q()
{
    int msg_q;
    if ((msg_q = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1)
        error("in msgget");
    return msg_q;
}

int get_sem_id()
{
    int sem_id;
    if ((sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0660)) == -1)
        error("in semget");
    semctl(sem_id, STATER, SETVAL, 0);
    semctl(sem_id, SCANNER, SETVAL, 1);
    return sem_id;
}

void scanner(bool base, Shm_path *shm_path, int sem_id, char *pathname, int id)
{
    struct dirent *dr;
    DIR *d;
    if ((d = opendir(pathname)) == NULL)
        error("in opendir");
    while ((dr = readdir(d)) != NULL)
    {
        if (dr->d_name[0] != '.')
        {
            if (dr->d_type == DT_REG)
            {
                WAIT(sem_id, SCANNER);
                memset(shm_path->pathname, 0, MAX_BUF);
                sprintf(shm_path->pathname, "%s/%s", pathname, dr->d_name);
                shm_path->id = id;
                shm_path->done = false;
                SIGNAL(sem_id, STATER);
            }
            else if (dr->d_type == DT_DIR)
            {
                char tmp[MAX_BUF];
                sprintf(tmp, "%s/%s", pathname, dr->d_name);
                scanner(false, shm_path, sem_id, tmp, id);
            }
        }
    }
    closedir(d);
    if (base)
    {
        WAIT(sem_id, SCANNER);
        shm_path->done = true;
        SIGNAL(sem_id, STATER);
    }
}

void stater(Shm_path *shm_path, int sem_id, int msg_q, int instances)
{
    usleep(200);
    Message *msg = (Message *)malloc(sizeof(Message));
    int cont = 0;
    while (1)
    {
        WAIT(sem_id, STATER);
        struct stat file;
        stat(shm_path->pathname, &file);
        msg->msgtyp = 1;
        memset(msg->data, 0, sizeof(msg->data));
        sprintf(msg->data, "%ld,%d,%d", file.st_blocks, shm_path->id, 0);

        if (shm_path->done)
            ++cont;
        if (cont == instances)
        {
            memset(msg->data, 0, sizeof(msg->data));
            sprintf(msg->data, "%ld,%d,%d", (long)(0), -1, 1);
            if (msgsnd(msg_q, msg, strlen(msg->data) + 1, IPC_NOWAIT) == -1)
                error("In snd");
            break;
        }
        if (msgsnd(msg_q, msg, strlen(msg->data) + 1, IPC_NOWAIT) == -1)
            error("In snd");
        SIGNAL(sem_id, SCANNER);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
        error("uso my-du-s [path-1] [path-2] [...]");
    int size[argc - 1];
    memset(size, 0, sizeof(size));

    // creazione memoria condivisa
    int id_shm = get_shm_id();
    Shm_path *shm_path = get_shm(id_shm);
    // creazione coda di messaggi
    int msg_q = get_msg_q();
    // creazione semafori
    int sem_id = get_sem_id();

    if (fork() == 0)
    {
        stater(shm_path, sem_id, msg_q, argc - 1);
        return 0;
    }

    for (int i = 0; i < argc - 1; i++)
    {
        if (!doesPathExist(argv[i + 1]))
            error("directory non esistente o non valida");
        if (fork() == 0)
        {
            scanner(true, shm_path, sem_id, argv[i + 1], i);
            return 0;
        }
    }

    Message rcvd_msg;
    char *blocks;
    char *id;
    char *done;
    while (1)
    {
        if (msgrcv(msg_q, &rcvd_msg, MAX_BUF, 0, 0) == -1)
            error("in rcv");
        blocks = strtok(rcvd_msg.data, ",");
        id = strtok(NULL, ",");
        done = strtok(NULL, "\n");
        size[atoi(id)] += atoi(blocks);
        if (atoi(done))
            break;
    }

    for (int i = 0; i < argc - 1; i++)
        printf("%s ha dimensione %d\n", argv[i + 1], size[i]);

    for (int i = 0; i < argc; i++)
        wait(NULL);
    // distruzione memoria condivisa
    shmctl(id_shm, IPC_RMID, NULL);
    // distruzione semaforo
    semctl(sem_id, 0, IPC_RMID);
    // distruzione coda di messaggi
    msgctl(msg_q, IPC_RMID, NULL);
}