#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#define BUF_SIZE 255

void error(char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

bool is_palindroma(char *line)
{
    int c = 0;
    for (int i = 0; i < round(strlen(line) / 2); i++)
    {
        if (line[i] == line[strlen(line) - i - 1])
            c++;
    }
    if (c == round(strlen(line) / 2))
        return true;
    else
        return false;
}

void reader(char *mapped, FILE *read, struct stat *s)
{ // invia il contenuto del file mappato in memoria alla pipe

    char input[BUF_SIZE];
    int c = 0;

    for (int i = 0; s->st_size; i++)
    {
        input[c] = mapped[i];
        c++; // input conterrà la stringa fino all carattere di invio
        if (c != 0 && mapped[i] == '\n')
        {                               // appena trovo il carattere di invio
            fprintf(read, "%s", input); // salvo la stringa nella pipe
            fflush(read);
            c = 0;
        }
    }

    fprintf(read, "%s", "<terminated>"); // Indico al processo padre quando può iniziare a leggere
    exit(0);
}

void padre(FILE *read, FILE *write)
{ // Leggo dalla pipe del reader e scrivo su quella del writer
    char line[BUF_SIZE];
    while (fgets(line, BUF_SIZE, read))
    {
        if (is_palindroma(line))
            fprintf(write, "%s\n", line);
    }
}

void writer(FILE *write)
{
    char line[BUF_SIZE];
    while (fgets(line, BUF_SIZE, write))
    { // Salviamo i dati
        if (is_palindroma(line))
            printf("%s\n", line);
    }
}

bool does_file_exists(char *path)
{
    struct stat s;
    if (stat(path, &s) == -1 || !S_ISREG(s.st_mode))
        return false;
    return true;
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        error("Uso <file.txt>\n");
    }

    if (!does_file_exists(argv[1]))
        error("File non esistente");

    struct stat s;
    if (!stat(argv[1], &s))
        error("Errore nella ricerca delle caratteristiche del file");

    int pipeDReader[2]; // Pipe usata tra il processo reader e il padre

    int pipeDWriter[2]; // Pipe usata tre il padre e il writer

    if (!pipe(pipeDReader) || !pipe(pipeDWriter))
        error("Pipe non creata con successo");

    int file; // apertura file per ottenere un descrittore come numero intero
    if (file = open(argv[1], O_RDWR | O_TRUNC | O_CREAT, 0660))
        error("Impossibile aprire il file");

    char *mapped; // mappatura del file in memoria
    if ((mapped = (char *)mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, file, 0)) == MAP_FAILED)
        error("Impossibile mappare il file in memoria");

    FILE *reader_read;
    FILE *reader_write;
    FILE *writer_read;
    reader_read = fdopen(pipeDReader[0], "r"); // apertura della pipe tra il reader e il padre
    reader_write = fdopen(pipeDReader[1], "w");
    writer_read = fdopen(pipeDWriter[1], "r"); // apertura della pipe tra il padre e il writer

    if (fork() == 0)
    {                                     // Creazione processo reader
        reader(mapped, reader_write, &s); // la pipe contiene i dati
    }

    FILE *write;
    write = fdopen(pipeDWriter[1], "w"); // apertura della pipe tra il padre e il writer

    padre(reader_read, write);

    if (fork() == 0)
    { // processo writer
        writer(writer_read);
    }

    wait(NULL);
    wait(NULL);

    close(file);
    close(pipeDReader[0]);
    close(pipeDReader[1]);
    close(pipeDWriter[0]);
    close(pipeDWriter[1]);
    fclose(reader_read);
    fclose(reader_write);
    fclose(writer_read);
}
