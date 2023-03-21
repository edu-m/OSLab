#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#define MAX_BUF 32

bool isPalindrome(char *word){
    //printf("%s ",word);
    int len = strlen(word);

    for (int i = 0; i < len/2; i++) {
        if (word[i] != word[len-1-i]) 
            return false;
    }
    return true;
}

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

void error(char *what)
{
    fprintf(stderr, "%s\n", what);
    exit(1);
}

bool doesFileExist(char *filename, struct stat* file)
{
    if (stat(filename, file) || !S_ISREG(file->st_mode))
        return false;
    return true;
}

void reader(char *filename, struct stat *file, int pipeDes_r)
{
    // close(pipeDes_r[0]);
    FILE *fpipe_r = fdopen(pipeDes_r, "w");
    int file_des = open(filename, O_RDONLY);
    char *lines;
    lines = (char*)mmap(NULL,file->st_size,PROT_READ,MAP_SHARED,file_des,0);
    if (lines == MAP_FAILED)
        error("in mappatura file su memoria");
    char buffer[MAX_BUF];
    memset(buffer,0,MAX_BUF);
    char curr[2];
    for(int i=0;i<file->st_size;i++){
        
        if(lines[i] != '\n')
            {
                curr[0] = lines[i];
                curr[1] = '\0';
                strcat(buffer,curr);
            }
        else 
        {
            fprintf(fpipe_r,"%s\n",buffer);
            memset(buffer,0,MAX_BUF);
        }
    }
    fprintf(fpipe_r,"-finito-");
}


void writer(int *pipeDes_w)
{
     close(pipeDes_w[1]);
    FILE *fpipe_w = fdopen(pipeDes_w[0], "r");
    char line[MAX_BUF];
   
    while(1){
        fgets(line,MAX_BUF,fpipe_w);
        printf("%s",line);
        if(strcmp(line, "-finito-") == 0)
            break;
        
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
        error("uso: palindrome-filter <input file>");
    struct stat file;
    if (!doesFileExist(argv[1], &file))
        error("file non trovato o non accessibile");
    
    int pipeDes_r[2];
    int pipeDes_w[2];
    if (pipe(pipeDes_r) == -1)
        error("in creazione pipe r");
     if (pipe(pipeDes_w) == -1)
         error("in creazione pipe w");

    if (fork() == 0)
    {
        reader(argv[1],&file, pipeDes_r[1]);
        return 0;
    }
    if (fork() == 0)
        {
            writer(pipeDes_w);
            return 0;
        }

    FILE *fpipe_r = fdopen(pipeDes_r[0], "r");
    FILE *fpipe_w = fdopen(pipeDes_w[1], "w");
    char line[MAX_BUF];
    while(1){
        fgets(line,MAX_BUF,fpipe_r);
        fflush(fpipe_r);
        if(isPalindrome(trim(line)))
        {fprintf(fpipe_w,"%s\n",trim(line));
        printf("%s\n",trim(line));}
        if(strcmp(trim(line), "-finito-") == 0)
        {
            fprintf(fpipe_w,"-finito-");
            break;
        }
        
    } 
}