#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define MAX_BUF 32

typedef struct msg
{
    long msgtyp;
    char data[MAX_BUF];
} Messaggio;

typedef struct nodo
{
    char data[MAX_BUF];
    struct nodo *next;

} Nodo;

/*
Funzioni di manipolazione di lista
*/

Nodo *cercaNodo(Nodo *testa, char *target)
{
    if (testa == NULL)
        return NULL;
    for (Nodo *temp = testa; temp != NULL; temp = temp->next)
        if (strncmp(target, temp->data, strlen(temp->data) + 1) == 0)
            return temp;
    return NULL;
}

bool inserisci(Nodo **testa, char *data)
{
    if (cercaNodo(*testa, data) != NULL)
        return false;
    Nodo *nuovoNodo = (Nodo *)(malloc(sizeof(Nodo)));
    strncpy(nuovoNodo->data, data, strlen(data) + 1);
    nuovoNodo->next = NULL;

    if (testa == NULL)
        *testa = nuovoNodo;
    else
    {
        nuovoNodo->next = *testa;
        *testa = nuovoNodo;
    }
    return true;
}

// puliamo le strutture dati una volta terminata l'esecuzione
void eliminaLista(Nodo *testa)
{
    Nodo *ptr;
    for (ptr = testa; testa != NULL; ptr = testa)
    {
        testa = testa->next;
        free(ptr);
    }
}

// Funzione per rendere stringa lowercase
char *tolower_c(char *data)
{
    for (size_t i = 0; i < strlen(data) + 1; i++)
        data[i] = tolower(data[i]);
    return data;
}

/*
Creiamo un Messaggio contenente i dati caricati da lettura del file.
*/
Messaggio *creaMessaggio(char *input)
{
    Messaggio *newMessaggio = (Messaggio *)malloc(sizeof(Messaggio));
    newMessaggio->msgtyp = 1;
    strncpy(newMessaggio->data, input, strlen(input) + 1);
    return newMessaggio;
}

bool fileExists(char *fileName)
{
    struct stat fileStat;
    return !(stat(fileName, &fileStat) || !S_ISREG(fileStat.st_mode));
}

// Eliminiamo tutti i caratteri "spuri" dalla stringa in modo da ritornare solo le lettere
char *trim(char *line)
{
    size_t i;
    char *trimmed = (char *)(malloc(MAX_BUF));
    for (i = 0; i < strlen(line); i++)
    {
        if (isspace(line[i]) == 0)
            trimmed[i] = line[i];
    }
    trimmed[i + 1] = '\0';
    return trimmed;
}

// funzione di lettura
void readFile(int q, char *fileName)
{
    FILE *file;
    if ((file = fopen(fileName, "r")) == NULL)
    {
        fprintf(stderr, "Errore nell'apertura del file\n");
        exit(1);
    }
    char line[MAX_BUF];
    Messaggio *msg;
    while (!feof(file))
    {
        fgets(line, MAX_BUF, file);
        msg = creaMessaggio(trim(line));
        msgsnd(q, msg, strlen(msg->data) + 1, IPC_NOWAIT);
    }
    msg = creaMessaggio("-finito-");
    msgsnd(q, msg, strlen(msg->data) + 1, IPC_NOWAIT);
}

// funzione di output
void writer(int pipe)
{
    FILE *filePipe;

    if ((filePipe = fdopen(pipe, "r")) == NULL)
    {
        fprintf(stderr, "Errore nell'apertura della pipe\n");
        exit(1);
    }
    char line[MAX_BUF];
    while (1)
    {
        fgets(line, MAX_BUF, filePipe);
        if (strcmp(line, "-finito-") == 0) // riceviamo dati da P
            break;
        printf("Writer output: %s", line);
    }
}

int main(int argc, char **argv)
{
    Nodo *testa = NULL;
    FILE *filePipe;
    int pipeD[2]; // creo un array di int per il descrittore della pipe. [0] per lettura e [1] per scrittura
    int output;
    if (argc < 3)
    {
        fprintf(stderr, "utilizzo: merge-lists <file-1> <file-2>\n");
        exit(1);
    }
    if (!fileExists(argv[1]) || !fileExists(argv[2]))
    {
        fprintf(stderr, "File non esistente\n");
        exit(1);
    }

    int msg_queue = msgget(IPC_PRIVATE, IPC_CREAT | 0660);
    if (msg_queue == -1)
    {
        perror("msgget");
        exit(1);
    }

    // Creo il descrittore di pipe che verrà successivamente passato a fdopen
    if (pipe(pipeD) == -1)
    {
        fprintf(stderr, "Errore nella creazione della pipe\n");
        exit(1);
    }

    // Istanzio il processo R1
    if (fork() == 0)
    {
        readFile(msg_queue, argv[1]);
        return 0;
    }

    // Istanzio il processo R2
    if (fork() == 0)
    {
        readFile(msg_queue, argv[2]);
        return 0;
    }

    // Istanzio il processo W
    if (fork() == 0)
    {
        writer(pipeD[0]);
        return 0;
    }
    // apro lo stream di scrittura della pipe e inizializzo variabili e strutture dati
    filePipe = fdopen(pipeD[1], "w");
    Messaggio msg;
    char *line;
    int eof = 0;
    while (1)
    {
        msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), 0, 0);
        // se entrambe le funzioni hanno terminato le stringhe da inviare terminiamo l'esecuzione
        if (strcmp(msg.data, "-finito-") == 0)
        {
            if (++eof == 2)
                break;
            else
                continue;
        }
        if (inserisci(&testa, tolower_c(msg.data))) // se siamo in presenza di un duplicato, non inviamo alla pipe
            fprintf(filePipe, "%s\n", msg.data);    // inviamo alla pipe i dati non duplicati passati da R1 e R2
    }
    // al termine dell'esecuzione di R1 e R2 inviamo a writer l'ultimo messaggio in pipe in modo da farlo terminare a sua volta
    fprintf(filePipe, "-finito-");
    msgctl(msg_queue, IPC_RMID, NULL);
    eliminaLista(testa);
    fclose(filePipe);
}