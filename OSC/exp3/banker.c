#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
#define CEIL 10

int available[NUMBER_OF_RESOURCES];

int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {{7,5,3}, {3,2,2}, {6,0,2}, {3,3,3}, {4,3,3}};

int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_mutex_t requestMutex, releaseMutex;

void AvailableInfo()
{
    // FILE* fp;
    // fp = fopen("log.txt", "w+");
    // fprintf(fp, "available:");
    // for(int i = 0; i < NUMBER_OF_RESOURCES; i++) fprintf(fp, "%d ", available[i]);
    // fprintf(fp, "\n");
    printf("available:");
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) printf("%d ", available[i]);
    printf("\n");
}

void printRequest(int* request)
{
    printf("request:");
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        
        printf("%d ",request[i]);
    }
    printf("\n");
}

void printWork(int* work)
{
    printf("work:");
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        
        printf("%d ",work[i]);
    }
    printf("\n");
}

void printNeed(int id)
{
    printf("need[%d]:",id);
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        printf("%d ",need[id][i]);
    }
    printf("\n");

}

bool nogreater(int *a, int *b, int n)
{
    for (int i = 0; i < n; i++)
    {
        if(a[i] > b[i]) return false;
    }
    return true;
    
}

bool isSafe()
{
    bool finish[NUMBER_OF_CUSTOMERS];
    int work[NUMBER_OF_RESOURCES];
    memset(finish, false, sizeof finish);
    memcpy(work, available, sizeof work);
    // printf("======================\n");
    while (1)
    {
        int flag = 1;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        {
            // printNeed(i);
            // printWork(work);
            if(finish[i] == false && nogreater(need[i], work, NUMBER_OF_RESOURCES))
            {
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                {
                    work[j] += allocation[i][j];
                }
                flag = 0;
                finish[i] = true;
                // printf("finish%d ==true\n",i);
            }
        }
        if(flag) break;
        
    }
    // printf("======================\n");
    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        if(finish[i] == false) return false;
    }
    return true;
    

}


int request_resources(int customer_num, int request[])
{
    printRequest(request);
    printNeed(customer_num);

    if(!nogreater(request, need[customer_num], NUMBER_OF_RESOURCES)) return -1;
    if(!nogreater(request, available, NUMBER_OF_RESOURCES)) return -2;

    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) available[i] -= request[i];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) allocation[customer_num][i] += request[i];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) need[customer_num][i] -= request[i];
    
    if(isSafe()) return 0;
    else
    {
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++) available[i] += request[i];
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++) allocation[customer_num][i] -= request[i];
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++) need[customer_num][i] += request[i];
        return -3;
    }
    
}

int release_resources(int customer_num, int release[])
{
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) available[i] += release[i];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) allocation[customer_num][i] -= release[i];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) need[customer_num][i] += release[i];
    return 0;

}

void working()
{
    sleep(1);
}
int cnt = 0;
void* customer(void* data)
{
    int id = *(int*)data;
    while (1)
    {
        cnt++;
        
        int request[NUMBER_OF_RESOURCES];
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)   request[i] = rand() % CEIL;

        int res = 0;
        pthread_mutex_lock(&requestMutex);
        
        printf("=======start a new request from pid=%d==========\n",id);
        printf("  bef work ");AvailableInfo();

        res = request_resources(id, request);

        if(res == 0)
        {
            printf("  now work ");AvailableInfo(); 
        }

        if(res == 0) 
            printf("=======a request compelet (<safe>) ============!\n\n");
        else  
            printf("=====a request NO compelet (no safe %d) =======\n\n",res);    
        
        pthread_mutex_unlock(&requestMutex);

        working();
        
        if(res != 0) continue;
        

        pthread_mutex_lock(&releaseMutex);
        release_resources(id, request);
        pthread_mutex_unlock(&releaseMutex);
    }

}


int main(int argc, char** argv)
{
    if(argc < 4) 
    {
        printf("./a.out 10 5 7\n");
    }

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        available[i] = atoi(argv[i+1]);
        for(int j = 0; j < NUMBER_OF_CUSTOMERS; j++)
        {
            need[j][i] = maximum[j][i] - allocation[j][i];

        }
    }
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        printf("need[%d]:",i);
        for(int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            printf("%d ",need[i][j] );
        }
        printf("\n");
    }
    

    
    srand(time(NULL));
    pthread_mutex_init(&requestMutex, NULL);
    pthread_mutex_init(&releaseMutex, NULL);
    

    pthread_t pth[NUMBER_OF_CUSTOMERS]; 
    pthread_attr_t attr[NUMBER_OF_CUSTOMERS]; 
    // for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    // {
    //     pthread_attr_init(&attr[i]);
    //     // pthread_attr_setdetachstate(&attr[i],PTHREAD_CREATE_DETACHED);
    // }

    // int* para;
    // para = (int*)malloc(sizeof(int));
    // *para = 0;
    // if(pthread_create(&pth[3], NULL, customer,(void*)para) == 0)
    // {
    //     printf("customer  enter!\n");
    // }
    // else
    // {
    //     printf("enter fail !\n");
    // }
    // pthread_join(pth[3], NULL);
    int error = -1;
    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        int* para;
        para = (int*)malloc(sizeof(int));
        *para = i;
        if(pthread_create(&pth[i], NULL, customer,(void*)para) == 0)
        {
            // printf("customer %d enter!\n",i);
        }
        else
        {
            printf("enter fail %d!\n",error);
        }
    }
    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        pthread_join(pth[i], NULL);
}
