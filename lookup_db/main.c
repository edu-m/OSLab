#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#define BUF_SIZE 50

typedef struct messaggio
{
    long msgtyp;
    char data[BUF_SIZE];
} Messaggio;

typedef struct nodo
{
    char nome[BUF_SIZE];
    int valore;
    struct nodo *next;
} Nodo;

void inserisciInTesta(Nodo **testa, char *nome, int valore)
{
    Nodo *nuovoNodo = (Nodo *)(malloc(sizeof(Nodo)));
    strncpy(nuovoNodo->nome, nome, sizeof(nome));
    nuovoNodo->next = NULL;

    if (testa == NULL)
        *testa = nuovoNodo;
    else
    {
        nuovoNodo->next = *testa;
        *testa = nuovoNodo;
    }
}

Messaggio *creaMessaggio(char *input)
{
    Messaggio *messaggio = (Messaggio *)malloc(sizeof(Messaggio));
    strncpy(messaggio->data, input, strlen(input) + 1);
    messaggio->msgtyp = 1;
    return messaggio;
}

bool doesFileExist(char *file)
{
    struct stat fileStat;
    return !(stat(file, &fileStat) || !S_ISREG(fileStat.st_mode));
}

void in(char *file, int q, int id)
{
    if (!doesFileExist(file))
    {
        fprintf(stderr, "File non esistente o non utilizzabile.\n");
        exit(1);
    }
    FILE *query = fopen(file, "r");
    char line[BUF_SIZE];
    char buffer[BUF_SIZE * 2];
    Messaggio *messaggio;
    int counter = 0;
    while (!feof(query))
    {
        fgets(line, BUF_SIZE, query);

        for (int i = 0; i < BUF_SIZE; ++i)
            if (line[i] == '\n')
                line[i] = '\0';
        sprintf(buffer, "%d,%s", id, line);
        messaggio = creaMessaggio(buffer);
        printf("IN%d: Inviata Query n.%d %s\n", id, ++counter, line);
        // printf("%d\n", strlen(messaggio->data) + 1);
        msgsnd(q, messaggio, strlen(messaggio->data) + 1, IPC_NOWAIT);
    }
    sprintf(buffer, "%d,finito", id);
    messaggio = creaMessaggio(buffer);
    msgsnd(q, messaggio, strlen(messaggio->data) + 1, IPC_NOWAIT);
}

void db(char *file, int q1, int q2)
{
    if (!doesFileExist(file))
    {
        fprintf(stderr, "File DB non esistente o non utilizzabile.\n");
        exit(1);
    }
    Nodo *testa = NULL;
    char *nome;
    int valore;
    FILE *query = fopen(file, "r");
    char line[BUF_SIZE];
    Messaggio messaggioInput;

    while (!feof(query))
    {
        fgets(line, BUF_SIZE, query);
        nome = strtok(line, ":");
        valore = atoi(strtok(NULL, "\n"));
        inserisciInTesta(&testa, nome, valore);
    }

    bool eof[2] = {false};
    int idRicevuto;
    char *nomeRicevuto;
    while (!eof[0] || !eof[1])
    {
        msgrcv(q1, &messaggioInput, sizeof(messaggioInput) - sizeof(long), 0, 0);
        idRicevuto = atoi(strtok(messaggioInput.data, ","));
        nomeRicevuto = strtok(NULL, "\n");
        if (strcmp(nomeRicevuto, "finito") == 0)
            eof[idRicevuto - 1] = true;
        else
            printf("%d %s\n", idRicevuto, nomeRicevuto);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Uso: lookup-database <db-file> <query-file-1> <query-file-2>\n");
        exit(1);
    }

    int q1 = msgget(IPC_PRIVATE, IPC_CREAT | 0660);
    if (q1 == -1)
    {
        perror("msgget queue 1");
        exit(1);
    }
    int q2 = msgget(IPC_PRIVATE, IPC_CREAT | 0660);
    if (q2 == -1)
    {
        perror("msgget queue 2");
        exit(1);
    }

    if (fork() == 0)
    {
        in(argv[2], q1, 1);
    }

    if (fork() == 0)
    {
        in(argv[3], q1, 2);
    }

    if (fork() == 0)
    {
        db(argv[1], q1, q2);
    }
}
