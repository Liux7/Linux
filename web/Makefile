bin = ./bin
src = ./src/webserver.c
obj = $(bin)/server
flag = -lrt -lpthread
all:
	gcc $(src) -o $(obj)
sm:
	gcc $(src) -o $(obj) $(flag)
clean:
	rm $(obj)
