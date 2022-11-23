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

pthread_mutex_t readmutex, writemutex, buffmutex;

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
        int flag = 1;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        {
            if(finish[i] == false && nogreater(need, work, NUMBER_OF_RESOURCES))
            {
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                {
                    work[j] += allocation[i][j];
                    finish[i] = true;
                }
                flag = 0;
            }
        }
        if(flag) break;
        
    }
    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        if(finish[i] == false) return false;
    }
    return true;
    

}


int request_resources(int customer_num, int request[])
{
    if(!nogreater(request, need[customer_num], NUMBER_OF_CUSTOMERS)) return -1;
    if(!nogreater(request, available[customer_num], NUMBER_OF_CUSTOMERS)) return -1;

    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) available[i] -= request[i];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) allocation[customer_num][i] += request[i];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++) need[customer_num][i] -= request[i];

    if(isSafe()) return 0;
    else
    {
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++) available[i] += request[i];
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++) allocation[customer_num][i] -= request[i];
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++) need[customer_num][i] += request[i];
        return -1;
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
