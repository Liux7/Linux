#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

const int N = 100;

int inCircle = 0;
int range = 2.0;

void *runner(void *param)
{
	double x = ((double)rand()/(double)(RAND_MAX)*range) - 1;
	double y = ((double)rand()/(double)(RAND_MAX)*range) - 1;
	double dis = sqrt(x*x+y*y);
//	printf("%f %f %f\n",x, y, dis); 
	if(dis <= 1)
	{
		inCircle++;
	}
}

int main()
{

	pthread_t threads[N];

	int total = N;

	srand((unsigned int)time(NULL));

	struct timeval t0,t1;
	gettimeofday(&t0, NULL);
	for(int i = 0; i < total; i++)
	{
		pthread_create(&threads[i], NULL, runner, NULL);		
	}
	gettimeofday(&t1, NULL);

	for(int i = 0; i < total; i++)
	{
		pthread_join(threads[i], NULL);
	}


	double time = (t1.tv_sec - t0.tv_sec);
	printf("PAI:%f time:%f\n",4*inCircle/(1.0*total), time);

	return 0;
}
