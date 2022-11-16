#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
using namespace std;

int main()
{
    int p1;
    while((p1 = fork()) == -1);
    if(p1 == 0)
    {
        /*子进程*/
        int i = 0;
        printf("p1 sleep\n");
        while(1){
            printf("%d..\n", i++);
        }
    }
    else{
        /*父进程*/
        printf("go to parent\n");
        while(1){
            for(int i = 1; i <= 1000000; i++)
                printf("a\n");
            sleep(1);
            for(int i = 1; i <= 1000000; i++)
                cout << 'b' << endl;
        }
    }
}