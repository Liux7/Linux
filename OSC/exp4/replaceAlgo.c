#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifndef FRAMESIZE
    #define FRAMESIZE 3
#endif

#ifndef PAGESIZE
    #define PAGESIZE 10
#endif 

const int INF = 0x3f3f3f3f;

void genRandomPageRefer(int* array, int size)
{

    
    srand((unsigned int)time(NULL));
    for(int i = 0; i < size; i++)
    {
        array[i] = rand() % PAGESIZE;
    }
}

typedef struct LinkNode
{
    int data;
    int refer;
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

Node* findNodeClock(list* queue, int pageNum)
{
    Node* p = queue->head->next;
    Node* front = p;
    while (p != NULL)
    {
        if(p->data == pageNum)
        {
            return p;
        }
        p = p->next;
        if(p == front) break;
    }
    return NULL;
}

Node* findpreNode(list* queue, int pageNum)
{
    Node* p = queue->head;
    if(p->next->data == pageNum) return p;
    while (p->next != NULL)
    {
        if(p->next->data == pageNum)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

void popNode(list* queue)
{
    queue->size--;
    Node* p = queue->head;
    if(p->next != NULL)
    {
        Node* tmp = p->next;
        p->next = tmp->next;
        free(tmp);
    }
}
    
Node* frontNode(list* queue)
{
    return queue->head->next; 
}

void pushNode(list* queue, int pageNum)
{
    queue->size++;
    Node* p = queue->head;
    Node* tmp = (Node*)malloc(sizeof(Node));
    tmp->data = pageNum;
    tmp->next = NULL;
    while(p->next != NULL) p = p->next;
    p->next = tmp;
}

void pushNodeOpt(list* queue, list* forecast[], int pageNum)
{
    
    if(queue->size == 0 )
    {
        pushNode(queue, pageNum);
    }
    else
    {
        queue->size++;
        Node* p = queue->head;
        Node* pNode = frontNode(forecast[p->next->data]);
        int pPos = pNode == NULL ? INF : pNode->data;
        Node* newNode = frontNode(forecast[pageNum]);
        int newPos = newNode == NULL ? INF : newNode->data;
        while(newPos < pPos)
        {
            
            p = p->next;
            if(p->next == NULL)
            {
                Node* tmp = (Node*)malloc(sizeof(Node));
                tmp->data = pageNum;
                tmp->next = NULL;
                p->next = tmp;
                return;
            }

            pNode = frontNode(forecast[p->next->data]);
            pPos = pNode == NULL ? INF : pNode->data;
            
        }

        Node* tmp = (Node*)malloc(sizeof(Node));
        tmp->data = pageNum;
        tmp->next = p->next;
        p->next = tmp;


        
    }
}


void debugList(list* queue)
{
    printf("size:%d ", queue->size);
    printf("head->");
    Node* p = queue->head->next;
    while(p != NULL)
    {
        printf("%d->", p->data);
        p = p->next;
    }
    printf("\n");
}

void debugOPT(list* queue, list* forecast[])
{
    printf("size:%d ",queue->size);
    printf("head->");
    Node* p = queue->head->next;
    while(p != NULL)
    {
        Node* tmp = frontNode(forecast[p->data]);
        int tmpPos = tmp == NULL ? INF : tmp->data;
        printf("%d(%d)->", p->data, tmpPos);
        p = p->next;
    }
    printf("\n");
}

void debugLRU(list* queue, int* recent)
{
    printf("size:%d ", queue->size);
    printf("head->");
    Node* p = queue->head->next;
    while(p != NULL)
    {
        printf("%d(%d)->", p->data, recent[p->data]);
        p = p->next;
    }
    printf("\n");
}

void debugClock(list* queue)
{
    printf("size:%d ", queue->size);
    printf("head->");
    if(queue->head->next == NULL) return;
    Node* p = queue->head->next->next;
    Node* front = p;
    while(p != NULL)
    {
        printf("%d(%d)->", p->data, p->refer);
        p = p->next;
        if(p == front) break;
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

void deleteList(list* queue)
{
    Node* p = queue->head;
    while(p != NULL)
    {
        Node* tmp = p;
        p = p->next;
        free(tmp);
    }
    free(queue);
}

int getPageFaultFIFO(int* pageRefer, int size)
{
    int pageFault = 0;

    list* queue = createList();
    for(int i = 0; i < size; i++)
    {
        Node* tmp = findNode(queue, pageRefer[i]);
        
        #ifdef DEBUG  
        printf("==============\n"); debugList(queue); printf("now page refer:\n");
        for(int j = 0; j < size; j++)
        {
            if(i == j) printf("[%d] ",pageRefer[j]);    
            else printf("%d ",pageRefer[j]);
            
        }
        printf("\n");
        #endif
        
        if(tmp != NULL)
        {
            #ifdef DEBUG
            printf("hit!\n");
            #endif
            continue;
        }    
        else
        {
            #ifdef DEBUG
            printf("miss!\n");
            #endif

            pageFault++;
            if(queue->size >= FRAMESIZE)
            {
                popNode(queue);
            }            
            pushNode(queue, pageRefer[i]);
            
        }
    }
    deleteList(queue);
    return pageFault;
}

int getPageFaultOPT(int *pageRefer, int size)
{
    int pageFault = 0;
    list* forecast[PAGESIZE];
    list* queue = createList();
    for(int i = 0; i < PAGESIZE; i++) {forecast[i] = createList(); forecast[i]->size = i;}

    for(int i = 0; i < size; i++)
    {
        pushNode(forecast[pageRefer[i]], i);
    }
    for(int i = 0; i < size; i++)
    {
        Node* tmp = findNode(queue, pageRefer[i]);

        #ifdef DEBUG  
        printf("==============\n"); debugOPT(queue, forecast); 
        printf("now page refer:\n");

        for(int j = 0; j < size; j++)
        {
            if(i == j) printf("[%d] ",pageRefer[j]);    
            else printf("%d ",pageRefer[j]);
            
        }
        printf("\n");
        #endif

        popNode(forecast[pageRefer[i]]);
        if(tmp != NULL)
        {
            #ifdef DEBUG
            printf("hit!\n");
            #endif
            
            Node* pre = findpreNode(queue, pageRefer[i]);
            Node* p = pre->next;
            pre->next = p->next;
            free(p);
            queue->size--;
        }    
        else
        {
            pageFault++;
            if(queue->size >= FRAMESIZE)
            {
                popNode(queue);
                
            }     
            #ifdef DEBUG
            printf("miss!\n");
            #endif
        }
        pushNodeOpt(queue, forecast, pageRefer[i]);
    }
    for(int i = 0; i < PAGESIZE; i++) deleteList(forecast[i]);
    return pageFault;

}

void sortListLRU(list* queue, int* recent)
{
    // pushNode(queue, pageNum);
    if(queue->size <= 1) return;

    for(int i = 0; i < queue->size - 1; i++)
    {
        Node* p = queue->head->next;
        Node* pre = queue->head;
        while(p->next != NULL)
        {
            if(recent[p->data] > recent[p->next->data])
            {
                pre->next = p->next;
                p->next = pre->next->next;
                pre->next->next = p;

                pre = pre->next;
            }
            else break;

        }

    }


    
}

int getPageFaultLRU(int* pageRefer, int size)
{
    int pageFault = 0;
    int recent[PAGESIZE];
    memset(recent, -1, sizeof(recent));
    // int* recent = (int*)malloc(sizeof(int) * PAGESIZE);
    // for(int i = 0; i < PAGESIZE; i++) recent[i] = -1;
    list* queue = createList();
    for(int i = 0; i < size; i++)
    {
        Node* tmp = findNode(queue, pageRefer[i]);

        #ifdef DEBUG  
        printf("==============\n"); debugLRU(queue, recent); 
        printf("now page refer:\n");
        for(int j = 0; j < size; j++)
        {
            if(i == j) printf("[%d] ",pageRefer[j]);    
            else printf("%d ",pageRefer[j]);
        }
        printf("\n");
        #endif

        
        if(tmp != NULL)
        {
            #ifdef DEBUG
            printf("hit!\n");
            #endif
          
        }
        else
        {
            sortListLRU(queue, recent);
            pageFault++;
            if(queue->size >= FRAMESIZE)
            {
                popNode(queue);
            }     
            
            
            #ifdef DEBUG
            
            printf("miss!\n");
            #endif
            // pushNodeLRU(queue, recent, pageRefer[i]);
            pushNode(queue, pageRefer[i]);
        }
        // debugLRU(queue, recent);
        #ifdef DEBUG
        debugList(queue);
        #endif
        recent[pageRefer[i]] = i;
    }

    return pageFault;
    
}

void pushNodeClock(list* queue, int pageNum)
{
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = pageNum;
    newNode->refer = 1;
    if(queue->size == 0)
    {
        queue->head->next = newNode;
        newNode->next = newNode;   
    }
    else
    {   
        newNode->next = queue->head->next->next;
        queue->head->next->next = newNode;
        queue->head->next = newNode;
    }
    queue->size++;
    
}

void popNodeClock(list* queue)
{
    if(queue->size == 0) return;
    queue->size--;
    if(queue->size == 1) 
    {
        Node* tmp = queue->head->next;
        free(tmp);
        return;
    }
    // Node* pre = queue->head;
    Node* p = queue->head->next;
    while(p->next->refer == 1)
    {
        p->next->refer = 0;
        p = p->next;
    }
    Node* tmp = p->next;
    p->next = tmp->next;
    free(tmp);

}

int getPageFaultCLOCK(int* pageRefer, int size)
{
    int pageFault = 0;
    list* queue = createList();

    for(int i = 0; i < size; i++)
    {
        Node* tmp = findNodeClock(queue, pageRefer[i]);

        #ifdef DEBUG  
        printf("==============\n"); debugClock(queue); 
        printf("now page refer:\n");
        for(int j = 0; j < size; j++)
        {
            if(i == j) printf("[%d] ",pageRefer[j]);    
            else printf("%d ",pageRefer[j]);
        }
        printf("\n");
        #endif

        if(tmp != NULL)
        {
            #ifdef DEBUG
            printf("hit!\n");
            #endif
            tmp->refer = 1;
        }
        else
        {
            pageFault++;
            if(queue->size >= FRAMESIZE)
            {
                popNodeClock(queue);
            }     
            
            
            #ifdef DEBUG
            printf("miss!\n");
            #endif
            // pushNodeLRU(queue, recent, pageRefer[i]);
            pushNodeClock(queue, pageRefer[i]);
        }
        
    }
    return pageFault;
}

int main()
{

    int n = 20;
    int* pageRefer = (int*)malloc(n * sizeof(int));
    genRandomPageRefer(pageRefer, n);
    pageRefer[0] = 7;
    pageRefer[1] = 2;
    pageRefer[2] = 3;
    pageRefer[3] = 1;
    pageRefer[4] = 2;
    pageRefer[5] = 5;
    pageRefer[6] = 3;
    pageRefer[7] = 4;
    pageRefer[8] = 6;
    pageRefer[9] = 7;
    pageRefer[10] = 7;
    pageRefer[11] = 1;
    pageRefer[12] = 0;
    pageRefer[13] = 5;
    pageRefer[14] = 4;
    pageRefer[15] = 6;
    pageRefer[16] = 2;
    pageRefer[17] = 3;
    pageRefer[18] = 0;
    pageRefer[19] = 1;





    printf("page reference string:\n");
    for(int i = 0; i < n; i++)
    {
        printf("%d ", pageRefer[i]);
    }
    printf("\n");



    printf("FIFO:  %-5d Total: %d\n\n\n", getPageFaultFIFO(pageRefer, n), n);
    printf("OPT:   %-5d Total: %d\n\n\n", getPageFaultOPT(pageRefer,  n), n);
    printf("LRU:   %-5d Total: %d\n\n\n", getPageFaultLRU(pageRefer,  n), n);
    printf("CLOCK: %-5d Total: %d\n\n\n", getPageFaultCLOCK(pageRefer,  n), n);

    
    
    

}






void test()
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