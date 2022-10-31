#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#define SIZE 5

int  nums[SIZE]={0,1,2,3,4};

int main()
{
	int i;
	pid_t pid;
	pid = fork();
	if(pid == 0)
	{
		for(int i = 0; i < SIZE; i++)
		{
			nums[i] *= -i;
			printf("child: %d ",nums[i]);

		}
	}
	else if(pid > 0){
		wait(NULL);
		for(int i = 0; i < SIZE; i++)
		{
			printf("parent: %d ", nums[i]);
		}
	}
	return 0;
}
