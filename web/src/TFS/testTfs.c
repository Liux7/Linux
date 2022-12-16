#include <stdio.h>
#include "tfs.h"

int main()
{
    printf("create block\n");
    Block* bk = createBlock(1);
    printf("wirte file\n");
    writeFile(bk, "1.txt");


    printf("wirte file\n");
    writeFile(bk, "2.txt");

    printf("write img\n");
    writeFile(bk, "3.jpeg");

    printf("read 2 file\n");
    char* a = readFile(bk, "2.txt");
    printf("read file:%s\n", a);

    char* b = readFile(bk, "1.txt");
    printf("read file:%s\n", b);
    printf("delete block\n");
    deleteBlock(bk);
}