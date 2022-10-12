helper.o:
	gcc src/helper.c -D _DEBUG -o bin/helper.o -c -g

client: helper.o
	gcc src/client.c -D _DEBUG -o bin/client.o -c -g
	gcc -o client bin/client.o bin/helper.o -g


server: helper.o
	gcc -pthread src/server.c -D _DEBUG -o bin/server.o -c
	gcc -pthread -o server bin/server.o bin/helper.o 

clean:
	rm -rf bin
	rm client
	rm server