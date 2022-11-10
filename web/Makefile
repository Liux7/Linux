bin = ./bin
src = ./src
obj = $(bin)/server
flag = -lrt -lpthread
all:
	gcc $(src)/webserver.c -o $(obj)
sm:
	gcc $(src)/webserver.c -o $(obj) $(flag)
clt:
	gcc $(src)/client.c -o client
ptd:
	gcc -std=gnu99 -g $(src)/webserver.c -o $(obj) $(flag)

ptdpl:threadpool.o webserver_threadpool.o
	gcc  $(bin)/threadpool.o $(bin)/webserver_threadpool.o  -o  $(obj) $(flag)

webserver_threadpool.o:
	gcc -c $(src)/webserver.c $(flag) -o $(bin)/webserver_threadpool.o

threadpool.o:
	gcc -c $(src)/threadpool.c  $(flag) -o $(bin)/threadpool.o

clean:
	rm $(obj)
