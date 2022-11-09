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

clean:
	rm $(obj)
