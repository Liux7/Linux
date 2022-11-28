bin = ./bin
src = ./src
webc = $(src)/webserver.c
obj = $(bin)/server
flag = -lrt -lpthread
all:
	gcc $(webc) -o $(obj)
sm:
	gcc $(webc) -o $(obj) $(flag)
clt:
	gcc $(src)/client.c -o client
ptd:
	gcc -std=gnu99 -g $(webc) -o $(obj) $(flag)

ptdpl:threadpool.o webserver_threadpool.o
	gcc  $(bin)/threadpool.o $(bin)/webserver_threadpool.o  -o  $(obj) $(flag)

webserver_threadpool.o:
	gcc -c $(webc) $(flag) -o $(bin)/webserver_threadpool.o

threadpool.o:
	gcc -c $(src)/threadpool.c  $(flag) -o $(bin)/threadpool.o

multiptdpl:
	gcc $(webc) -o $(obj) $(flag)

cache: jwHash.o
	gcc $(webc) $(bin)/$^ -I$(src) -o $(obj) $(flag) -DHASHTHREADED

cache2:
	gcc $(webc) -o $(obj) $(flag)

jwHash.o:
	gcc -c $(src)/jwHash.c -o $(bin)/$@ 

clean:
	rm $(obj)
	rm $(bin)/*.o
