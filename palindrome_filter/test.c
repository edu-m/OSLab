#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

bool isPalindrome(char *word){
    int len = strlen(word);
    for (int i = 0; i < len/2; i++) {
        if (word[i] != word[len-1-i]) 
            return false;
    }
    return true;
}

int main(){
    printf("%d\n",isPalindrome("osso"));
}