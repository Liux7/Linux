#ifndef _TFS_H_
#define _TFS_H_

#include <jwHash.h>

#define MAX_FILE_NUM 10

typedef long int int;

typedef struct fileInfo
{
    int id;
    int offset;
    int size;
} fileInfo;


typedef struct Block
{
    int blockID;
    char* blockName;
    int used;
    fileInfo* infos[MAX_FILE_NUM];
    jwHashTable* hashTable;
} Block;

Block* createBlock(int id);

void writeFile(Block* bk, char* filename);

char* readFile(Block* bk, char* filename);

void deleteBlock(Block* bk);




#endif