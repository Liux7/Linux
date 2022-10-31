#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
	pid_t pid;
	fork();
//	printf("==1===\n");
	fork();
//	printf("==2===\n");
	fork();
//	printf("==3===\n");


	printf("pid:%d fpid:%d\n", getpid(), getppid());

	return 0;

}
