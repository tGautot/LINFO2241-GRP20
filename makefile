helper.o:
	# gcc src/helper.c -D _DEBUG -o bin/helper.o -c -g 
	gcc src/helper.c -o bin/helper.o -c -g 

client: helper.o
	gcc -pthread src/client.c -D _DEBUG -o bin/client.o -c -g
	# gcc -pthread src/client.c -o bin/client.o -c -g
	gcc -pthread -o client bin/client.o bin/helper.o -g


server: helper.o
	# gcc -pthread src/server.c -D _DEBUG -o bin/server.o -c -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -pthread src/server.c -o bin/server.o -c -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -pthread -o server bin/server.o bin/helper.o 


# server fully optimized (loop unrolling and matrix multiplication)
server-optim: helper.o
	# gcc src/server-optim.c -D _DEBUG -o bin/server-optim.o -c -g -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -pthread src/server-optim.c -o bin/server-optim.o -c -DOPTIM=1 -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -o server-optim bin/server-optim.o bin/helper.o 

# server with only optimized matrix multiplication
server-mem: helper.o
	gcc -pthread src/server-optim.c -o bin/server-mem.o -c -g -DOPTIM=2 -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -o server-mem bin/server-mem.o bin/helper.o 

# server without optimization
server-basic: helper.o
	gcc -pthread src/server-optim.c -o bin/server-basic.o -c -g -DOPTIM=0 -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -o server-basic bin/server-basic.o bin/helper.o 

server-loop: helper.o
	gcc -pthread src/server-optim.c -o bin/server-loop.o -c -g -DOPTIM=10 -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 
	gcc -o server-loop bin/server-loop.o bin/helper.o 


clean:
	rm -rf bin
	rm client
	rm server

#-mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2
