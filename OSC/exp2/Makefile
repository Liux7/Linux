obj = pai
flag = -fopenmp -lm -lpthreads

omp:
	gcc ompPai.c -o omp$(obj) $(flag)
ptd:
	gcc ptdPai.c -o ptd$(obj) $(flag)
clean:
	rm *$(obj)
