workplace = ~/Desktop/Linux/web
bin = $(workplace)/bin
jH = $(workplace)/src/jwHash

all:testTfs.o
	gcc $(bin)/tfs.o $(bin)/testTfs.o $(bin)/jwHash.o -o test
	
testTfs.o: tfs.o
	gcc -c testTfs.c -o $(bin)/$@

tfs.o: jwHash.o
	gcc -c tfs.c -o $(bin)/$@

jwHash.o:
	gcc -c $(jH)/jwHash.c -o $(bin)/$@

clean:
	rm $(bin)/*.o