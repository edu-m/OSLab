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

bool doesFileExist(char *file)
{
    struct stat fileStat;
    return !(stat(file, &fileStat) || !S_ISREG(fileStat.st_mode));
}

void in(char *file)
{
    if (!doesFileExist(file))
    {
        fprintf(stderr, "File non esistente o non utilizzabile.\n");
        exit(1);
    }
    FILE *query = fopen(file, "r");
    char line[BUF_SIZE];
    while (!feof(query))
    {
        // printf("%s", fgets(line, BUF_SIZE, query));
    }
}

void db(char *file)
{
    Nodo *testa = NULL;
    char *nome;
    int valore;
    if (!doesFileExist(file))
    {
        fprintf(stderr, "File DB non esistente o non utilizzabile.\n");
        exit(1);
    }

    FILE *query = fopen(file, "r");
    char line[BUF_SIZE];
    while (!feof(query))
    {
        fgets(line, BUF_SIZE, query);
        nome = strtok(line, ":");
        valore = atoi(strtok(NULL, "\n"));
        inserisciInTesta(&testa, nome, valore);
    }

    for (Nodo *temp = testa; temp != NULL; temp = temp->next)
    {
        printf("Print dal DB: %s\n", temp->nome);
    }
}

int main(int argc, char *argv[])
{

    if (argc < 4)
    {
        fprintf(stderr, "Uso: lookup-database <db-file> <query-file-1> <query-file-2>\n");
        exit(1);
    }

    if (fork() == 0)
    {
        db(argv[1]);
    }

    if (fork() == 0)
    {
        in(argv[2]);
    }

    if (fork() == 0)
    {
        in(argv[3]);
    }
}