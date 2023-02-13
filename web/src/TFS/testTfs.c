#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "tfs.h"

void getname(int i, char* c)
{
    
    int pos = 0;
    if(i < 10)
    {
        c[pos++] = '0' + i;
    }
    else if(i>=10 && i <= 99)
    {
        c[pos++] = '0' + i/10;
        c[pos++] = '0' + i%10;
    }
    else
    {
        c[pos++] = '0' + i/100;
        c[pos++] = '0' + i%100/10;
        c[pos++] = '0' + i%100%10;
    }
    c[pos++]='.';
    c[pos++]='j';
    c[pos++]='p';
    c[pos++]='e';
    c[pos++]='g';
    c[pos++]='\0';
    
}

int main(int argc, char** argv)
{
    printf("create block\n");
    Block* bk = createBlock(1);
    // printf("wirte file\n");
    // writeFile(bk, "1.txt");

    int N = atoi(argv[1]);
    // printf("wirte file\n");
    // writeFile(bk, "2.txt");

    // printf("write img\n");
    // writeFile(bk, "3.jpeg");

    // printf("read 2 file\n");

    // char* a = readFile(bk, "2.txt");
    // printf("read file:%s\n", a);
    
    // printf("read file:%s\n", b);

    // readbinFile(bk, "3.jpeg", "skadi.jpeg");
    
    struct timeval t0,t1,t2,t3;
    char* c = (char*)malloc(20*sizeof(char));
    gettimeofday(&t0, NULL);
    for(int j = 0; j < N; j++)
    for(int i = 1; i <= 100; i++)
    {
        
        char dir[20] = "../arknights/";
        getname(i, c);
        strcat(dir, c);

        // printf("%s\n", dir);
        char* b = readFile(bk, dir);
    }
    gettimeofday(&t1, NULL);
    double time1 = (t1.tv_usec - t0.tv_usec)/1000 + (t1.tv_sec - t0.tv_sec)*1000;
    printf("%lf ", time1);

    gettimeofday(&t2, NULL);
    for(int j = 0; j < N; j++)
    for(int i = 1; i <= 100; i++)
    {
        char dir[20] = "../arknights/";
        getname(i, c);
        strcat(dir, c);

        // printf("%s\n", dir);

        FILE *fp = fopen(dir, "r");
        char* b;
        
        while(!feof(fp))
        {
            fgetc(fp);
        }
    }
    gettimeofday(&t3, NULL);
    double time2 = (t3.tv_usec - t2.tv_usec)/1000 + (t3.tv_sec - t2.tv_sec)*1000;
    printf("%lf\n", time2);
    


    printf("delete block\n");
    deleteBlock(bk);
}