#include "tfs.h"
#include <stdio.h>

#define RMALLOC(type,n) (type*)malloc(sizeof(type)*(n))
#define MALLOC(p,type,n) type*p = RMALLOC(type, n)

Block* createBlock(int id)
{
    Block* newBk;
    MALLOC(newBk, Block, 1);

    
    newBk->blockID = id;
    newBk->hashTable = create_hash(MAX_FILE_NUM);
    newBk->used = 0;
    return newBk;
}

void writeFile(Block* bk, char* filename)
{
    int fileid = bk->used;
    add_int_by_str(bk->hashTable, filename, fileid);
    bk->infos[fileid]->id = fileid;

    

}

char* readFile(Block* bk, char* filename)
{

}

void deleteBlock(Block* bk)
{

}