obj = ngx_palloc.o ngx_alloc.o 
inc = ./
dep = ngx_core.h ngx_config.h

all: test.o 
	gcc $^ $(obj) -o test -I$(inc)

test.o: test.c $(obj)
	gcc -c test.c -I$(inc)

ngx_palloc.o: ngx_palloc.h $(dep)
	gcc -c ngx_palloc.c -I$(inc)

ngx_alloc.o: ngx_alloc.h $(dep)
	gcc -c ngx_alloc.c -I$(inc)

.PHONY: clean

clean:
	rm *.o

