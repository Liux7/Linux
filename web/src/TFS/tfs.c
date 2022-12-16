
#include <stdio.h>
#include <stdlib.h>
#include "tfs.h"


#define RMALLOC(type,n) (type*)malloc(sizeof(type)*(n))
#define MALLOC(p,type,n) type*p = RMALLOC(type, n)

Block* createBlock(lint id)
{
    Block* newBk = (Block*)malloc(sizeof(Block));
    newBk->infos = (fileInfo**)malloc(sizeof(fileInfo*));
    *newBk->infos = (fileInfo*)malloc(MAX_FILE_NUM * sizeof(fileInfo));
    newBk->blockName = (char*)malloc(5);
    newBk->blockName = "T1";
    newBk->fp = fopen(newBk->blockName, "w+");
    newBk->blockID = id;
    newBk->hashTable = create_hash(MAX_FILE_NUM);
    newBk->used = 0;
    return newBk;
}

void writeFile(Block* bk, char* filename)
{

    lint fileid = bk->used;
    bk->used++;
    printf("used%ld\n", bk->used);
    add_int_by_str(bk->hashTable, filename, fileid);
    (*bk->infos)[fileid].id = fileid;
    FILE* fpr = fopen(filename, "r");
    // FILE* fpw = fopen(bk->blockName, "a+");
    FILE* fpw = bk->fp;
    printf("used%ld\n", bk->used);
    if(!fpr){printf("error when read file\n"); return ;}

    if(!fpw){printf("error when open block\n"); return ;}
    lint len = 0;
    while(!feof(fpr))
    {
        fputc(fgetc(fpr), fpw);
        len++;
    }
    (*bk->infos)[fileid].size = len;
    if(fileid != 0) 
        (*bk->infos)[fileid].offset = (*bk->infos)[fileid-1].offset + (*bk->infos)[fileid-1].size;

    fclose(fpr);
    return ;
}



char* readFile(Block* bk, char* filename)
{
    int fileid;
    get_int_by_str(bk->hashTable, filename, &fileid);
    // FILE* fpbk = fopen(bk->blockName, "r");
    FILE* fpbk = bk->fp;
    if(!fpbk){printf("error when open block\n");return NULL;}

    fseek(fpbk, (*bk->infos)[fileid].offset, SEEK_SET);

    lint size = (*bk->infos)[fileid].size;
    printf("offset:%ld size%ld\n", (*bk->infos)[fileid].offset, size);
    
    char* ret = (char*)malloc(size+5);
    int i = 0;
    while(!feof(fpbk))
    {
        ret[i++] = fgetc(fpbk);
        if(i == size) break;
    }
    ret[size - 1] = '\0';
    return ret;
}

void readbinFile(Block* bk, char* filename, char* newName)
{
    int fileid;
    get_int_by_str(bk->hashTable, filename, &fileid);
    FILE* fpbk = bk->fp;
    FILE* fpnf = fopen(newName, "w");

    fseek(fpbk, (*bk->infos)[fileid].offset, SEEK_SET);
    lint size = (*bk->infos)[fileid].size;
    int i = 0;
    while (i != size)
    {
        fputc(fgetc(fpbk), fpnf);
        i++;
    }
    


}

void deleteBlock(Block* bk)
{
    fclose(bk->fp);
    free(bk->hashTable);
    free(bk->infos);
    free(*bk->infos);
    free(bk);
}