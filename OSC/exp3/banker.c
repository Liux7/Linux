#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
#define CEIL 10

int available[NUMBER_OF_RESOURCES];

int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {{7,5,3}, {3,2,2}, {9,0,2}, {2,2,2}, {4,3,3}};

int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

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

    while (1)
    {
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        {
            if(finish[i] == false && nogreater(need, work, NUMBER_OF_RESOURCES))
            {
                for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
                {
                    
                }
                
            }
        }
        
    }
    

}


int request_resources(int customer_num, int request[])
{

}

int release_resources(int customer_num, int release[])
{

}

void working()
{
    sleep(1);
}

void customer(int id)
{
    int request[NUMBER_OF_RESOURCES];
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        request[i] = rand()/CEIL;
    }

    request_resources(id, request);
    working();
    release_resources(id, request);
    

}

void initmaximum()
{
    
}
pthread_mutex_t readmutex, writemutex, buffmutex;

int main(int argc, char** argv)
{
    if(argc < 4) 
    {
        printf("./a.out 10 5 7\n");
    }

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        available[i] = atoi(argv[i+1]);
    }

    // initmaximum();
    srand(time(NULL));
    pthread_mutex_init(&readmutex, NULL);
    pthread_mutex_init(&writemutex, NULL);
    pthread_mutex_init(&buffmutex, NULL);

    pthread_t pth[NUMBER_OF_CUSTOMERS]; 
    pthread_attr_t attr[NUMBER_OF_CUSTOMERS]; 
    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        pthread_attr_init(&attr[i]);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    }

    int* para;
    para = (int*)malloc(sizeof(int));

    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    if(pthread_create(&pth[i], &attr[i], &customer,(void*)para))
    {
        printf("customer %d enter!\n",i);
    }
    else
    {
        printf("enter fail!\n",i);
    }
    
}
