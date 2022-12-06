#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifndef FRAMESIZE
    #define FRAMESIZE 3
#endif

#ifndef PAGESIZE
    #define PAGESIZE 5
#endif 

void genRandomPageRefer(int* array, int size)
{

    array = (int*)malloc(size * sizeof(int));
    srand((unsigned int)time(NULL));
    for(int i = 0; i < size; i++)
    {
        array[i] = rand() % PAGESIZE;
    }
}

typedef struct LinkNode
{
    int data;
    struct LinkNode *next;
} Node;
typedef struct LinkList
{
    Node* head;
    int size;
} list;

Node* findNode(list* queue, int pageNum)
{
    Node* p = queue->head->next;
    while (p != NULL)
    {
        if(p->data == pageNum)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

void popNode(list* queue)
{
    Node* p = queue->head;
    if(p->next != NULL)
    {
        Node* tmp = p->next;
        p->next = tmp->next;
        free(tmp);
    }
}    

void pushNode(list* queue, int pageNum)
{
    Node* p = queue->head;
    Node* tmp = (Node*)malloc(sizeof(Node));
    tmp->data = pageNum;
    tmp->next = NULL;
    while(p->next != NULL) p = p->next;
    p->next = tmp;
}

void debugList(list* queue)
{
    printf("head->");
    Node* p = queue->head->next;
    while(p != NULL)
    {
        printf("%d->", p->data);
        p = p->next;
    }
    printf("\n");
}

list* createList()
{
    list* queue = (list*)malloc(sizeof(list));
    queue->head = (Node*)malloc(sizeof(Node));
    queue->size = 0;
    queue->head->next = NULL;
    return queue;
}

int getPageFault(int* PageRefer, int size)
{
    int pageFault = 0;

    list* queue = createList();
    for(int i = 0; i < size; i++)
    {
        Node* tmp = findNode(queue, PageRefer[i]);
        if(tmp != NULL)
        {
            continue;
        }    
        else
        {
            pageFault++;
            if(queue->size >= FRAMESIZE)
            {
                popNode(queue);
            }            
            pushNode(queue, PageRefer[i]);
            
        }
    }
    return pageFault;
}


int main()
{
    list* test = createList();
    pushNode(test, 5);
    pushNode(test, 7);
    pushNode(test, 9);
    pushNode(test, 2);
    debugList(test);
    Node* tmp = findNode(test, 9);
    if(tmp != NULL) printf("find it \n");
    else printf("no find\n");
    popNode(test);
    debugList(test);
    popNode(test);
    debugList(test);
    popNode(test);
    debugList(test);
    tmp = findNode(test, 2);
    if(tmp != NULL) printf("find it \n");
    else printf("no find\n");
    

}
