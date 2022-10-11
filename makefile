helper.o:
	gcc src/helper.c -o bin/helper.o -c

client: helper.o
	gcc src/client.c -o bin/client.o -c
	gcc -o client bin/client.o bin/helper.o


server: helper.o
	gcc src/server.c -o bin/server.o -c
	gcc -o server bin/server.o bin/helper.o 

clean:
	rm -rf bin
	rm client
	rm server