obj = pai
flag = -fopenmp -lm

omp:
	gcc ompPai.c -o omp$(obj) $(flag)
ptd:
	gcc ptdPai.c -o ptd$(obj) -lpthread -lm
clean:
	rm *$(obj)
