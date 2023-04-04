#include <stdio.h>
#include <stdlib.h>

void init_card(int **s, int **d){
    for(int i=0;i<3;i++){
        for(int j=0;j<5;j++){
            d[i][j] = s[i][j];
        }
    }
}

int main(){
    int *cards[5][8];

    init_card(cards[0], cards[1]);
}