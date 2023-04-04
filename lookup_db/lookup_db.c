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
    strncpy(nuovoNodo->nome, nome, strlen(nome) + 1);
    nuovoNodo->next = NULL;
    nuovoNodo->valore = valore;

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

void out(int q)
{
    Messaggio msgRcv;
    int counterIN1 = 0;
    int counterIN2 = 0;
    int sommaIN1 = 0;
    int sommaIN2 = 0;

    char *nome;
    char *valore;
    char *id;

    while (1)
    {
        msgrcv(q, &msgRcv, sizeof(msgRcv) - sizeof(long), 0, 0);
        nome = strtok(msgRcv.data, ",");
        valore = strtok(NULL, ",");
        id = strtok(NULL, "\n");

        if (strcmp(nome, "finito"))
        {
            printf("Ricevuto finito!\n");
            break;
        }

        if (atoi(id) == 1)
        {
            printf("incremento IN1\n");
            counterIN1++;
            sommaIN1 += atoi(valore);
        }
        else
        {
            printf("incremento IN2\n");
            counterIN2++;
            sommaIN2 += atoi(valore);
        }
    }
    msgctl(q, IPC_RMID, NULL);
    printf("OUT: ricevuti n.%d valori validi per IN1 con totale %d\n", counterIN1, sommaIN1);
    printf("OUT: ricevuti n.%d valori validi per IN2 con totale %d\n", counterIN2, sommaIN2);
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

Nodo *trovaInLista(Nodo *testa, char *nomeRicevuto, int idRicevuto)
{
    for (Nodo *temp = testa; temp != NULL; temp = temp->next)
        if (strcmp(nomeRicevuto, temp->nome) == 0)
            return temp;
    return NULL;
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
    Messaggio *messaggioOutput;
    int numOfLines = 0;
    while (!feof(query))
    {
        fgets(line, BUF_SIZE, query);
        nome = strtok(line, ":");
        valore = atoi(strtok(NULL, "\n"));
        inserisciInTesta(&testa, nome, valore);
        ++numOfLines;
    }
    // printf("DB: letti n. %d record da file\n", numOfLines);
    bool eof[2] = {false};
    int idRicevuto;
    char *nomeRicevuto;
    char buffer[BUF_SIZE];
    while (!eof[0] || !eof[1])
    {
        msgrcv(q1, &messaggioInput, sizeof(messaggioInput) - sizeof(long), 0, 0);
        idRicevuto = atoi(strtok(messaggioInput.data, ","));
        nomeRicevuto = strtok(NULL, "\n");
        if (strcmp(nomeRicevuto, "finito") == 0)
            eof[idRicevuto - 1] = true;
        else
        { // procedo con la ricerca in struttura dati
            Nodo *cercato = trovaInLista(testa, nomeRicevuto, idRicevuto);
            if (cercato != NULL)
            {
                printf("DB: query '%s' da IN%d trovata con valore %d\n", nomeRicevuto, idRicevuto, cercato->valore);
                sprintf(buffer, "%s,%d,%d", nome, valore, idRicevuto);
                messaggioOutput = creaMessaggio(buffer);
                msgsnd(q2, messaggioOutput, strlen(messaggioOutput->data) + 1, IPC_NOWAIT);
            }
            else
                printf("DB: query '%s' da IN%d non trovata\n", nomeRicevuto, idRicevuto);
        }
    }
    msgctl(q1, IPC_RMID, NULL);
    sprintf(buffer, "%s,%d,%d", "finito", valore, idRicevuto);
    messaggioOutput = creaMessaggio(buffer);
    msgsnd(q2, messaggioOutput, strlen(messaggioOutput->data) + 1, IPC_NOWAIT);

    Nodo *ptr;
    for (ptr = testa; testa != NULL; ptr = testa)
    {
        testa = testa->next;
        free(ptr);
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
        db(argv[1], q1, q2);
        return 0;
    }

    if (fork() == 0)
    {
        in(argv[2], q1, 1);
        return 0;
    }

    if (fork() == 0)
    {
        in(argv[3], q1, 2);
        return 0;
    }

    if (fork() == 0)
    {
        out(q2);
        return 0;
    }
    sleep(1);
}
