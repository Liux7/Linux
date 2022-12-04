bin = ./bin
obj = ngx_palloc.o ngx_alloc.o 
inc = ./
dep = ngx_core.h ngx_config.h

all: test.o $(obj)
	gcc $(bin)/test.o $(bin)/ngx_alloc.o $(bin)/ngx_palloc.o -o $(bin)/test -I$(inc)

test.o: test.c 
	gcc -c test.c -I$(inc) -o $(bin)/$@

ngx_palloc.o: ngx_palloc.h $(dep)
	gcc -c ngx_palloc.c -I$(inc) -o $(bin)/$@

ngx_alloc.o: ngx_alloc.h $(dep)
	gcc -c ngx_alloc.c -I$(inc) -o $(bin)/$@

.PHONY: clean

clean:
	rm $(bin)/*.o

