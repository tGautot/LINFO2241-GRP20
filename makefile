helper.o:
	# gcc src/helper.c -D _DEBUG -o bin/helper.o -c -g 
	gcc src/helper.c -o bin/helper.o -c -g 

client: helper.o
	gcc -pthread src/client.c -D _DEBUG -o bin/client.o -c -g
	# gcc -pthread src/client.c -o bin/client.o -c -g
	gcc -pthread -o client bin/client.o bin/helper.o -g


server: helper.o
	gcc -pthread src/server.c -D _DEBUG -o bin/server.o -c -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	# gcc -pthread src/server.c -o bin/server.o -c 
	gcc -pthread -o server bin/server.o bin/helper.o 

server-optim: helper.o
	gcc -pthread src/server-optim.c -D _DEBUG -o bin/server-optim.o -c -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	# gcc -pthread src/server-optim.c -o bin/server-optim.o -c 
	gcc -pthread -o server-optim bin/server-optim.o bin/helper.o 

clean:
	rm -rf bin
	rm client
	rm server

#-mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2
