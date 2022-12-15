#include <stdio.h>
#include <ngx_core.h>
#include <sys/time.h>
const int S = 1, L = 10000, N = 300000;

int main()
{
	printf("test\n");
	ngx_pool_t *pool;

	struct timeval t0, t1, t2, t3, t4, t5, t6, t7;

	int *ngx;
	pool = ngx_create_pool(25*sizeof(int), NULL);
	double LtimeMalloc = 0, LtimeFree = 0, StimeMalloc = 0, StimeFree = 0;
	printf("%-10s %-10s %-10s %-10s %-10s %-10s\n","assignS", "freeS", "totalS", "assignL", "freeL", "totalL");
	
	for(int i = 0; i < N; i++)
	{
		gettimeofday(&t0, NULL);
		ngx = (int*)ngx_palloc(pool, S*sizeof(int));
		gettimeofday(&t1, NULL);

		StimeMalloc += (t1.tv_sec - t0.tv_sec) * 1e6 + t1.tv_usec - t0.tv_usec;

		gettimeofday(&t2, NULL);
		ngx_pfree(pool, ngx);
		gettimeofday(&t3, NULL);

		StimeFree += (t3.tv_sec - t2.tv_sec) * 1e6 + t3.tv_usec - t2.tv_usec;

		gettimeofday(&t4, NULL);
		ngx = (int*)ngx_palloc(pool, L*sizeof(int));
		gettimeofday(&t5, NULL);

		LtimeMalloc += (t5.tv_sec - t4.tv_sec) * 1e6 + t5.tv_usec - t4.tv_usec;

		gettimeofday(&t6, NULL);
		ngx_pfree(pool, ngx);
		gettimeofday(&t7, NULL);

		LtimeFree += (t7.tv_sec - t6.tv_sec) * 1e6 + t7.tv_usec - t6.tv_usec;
	}
	ngx_destroy_pool(pool);
	printf("%-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf\n", 
	StimeMalloc, StimeFree, StimeMalloc + StimeFree, LtimeMalloc, LtimeFree, LtimeMalloc + LtimeFree);

	int *gcc;

	LtimeMalloc = 0, LtimeFree = 0, StimeMalloc = 0, StimeFree = 0;
	for(int i = 0; i < N; i++)
	{
		gettimeofday(&t0, NULL);
		gcc = malloc(S*sizeof(int));
		gettimeofday(&t1, NULL);

		StimeMalloc += (t1.tv_sec - t0.tv_sec) * 1e6 + t1.tv_usec - t0.tv_usec;

		gettimeofday(&t2, NULL);
		free(gcc);
		gettimeofday(&t3, NULL);

		StimeFree += (t3.tv_sec - t2.tv_sec) * 1e6 + t3.tv_usec - t2.tv_usec;

		gettimeofday(&t4, NULL);
		gcc = malloc(L*sizeof(int));
		gettimeofday(&t5, NULL);

		LtimeMalloc += (t5.tv_sec - t4.tv_sec) * 1e6 + t5.tv_usec - t4.tv_usec;

		gettimeofday(&t6, NULL);
		free(gcc);
		gettimeofday(&t7, NULL);

		LtimeFree += (t7.tv_sec - t6.tv_sec) * 1e6 + t7.tv_usec - t6.tv_usec;
	}
	printf("%-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf\n", 
	StimeMalloc, StimeFree, StimeMalloc + StimeFree, LtimeMalloc, LtimeFree, LtimeMalloc + LtimeFree);

	return 0;
}
