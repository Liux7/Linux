#ifndef _TFS_H_
#define _TFS_H_
#include "../jwHash/jwHash.h"


#define MAX_FILE_NUM 10

typedef long int lint;

typedef struct fileInfo
{
    lint id;
    lint offset;
    lint size;
} fileInfo;


typedef struct Block
{
    lint blockID;
    FILE* fp;
    char* blockName;
    lint used;
    fileInfo** infos;
    jwHashTable* hashTable;
} Block;

Block* createBlock(lint id);

void writeFile(Block* bk, char* filename);

char* readFile(Block* bk, char* filename);

void readbinFile(Block* bk, char* filename, char* newName);

void deleteBlock(Block* bk);




#endif