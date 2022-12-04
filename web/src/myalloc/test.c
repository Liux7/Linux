#include <stdio.h>
#include <ngx_core.h>

const int N = 20;

int main()
{
	printf("test\n");
	ngx_pool_t *tmp;
	tmp = ngx_create_pool(10*sizeof(int), NULL);

	int *a = (int*)ngx_palloc(tmp, N*sizeof(int));
	for(int i = 0; i < N; i++)
	{
		a[i] = i;
		printf("%d ", a[i]);
	}
	printf("\n");

	ngx_destroy_pool(tmp);

	return 0;
}
