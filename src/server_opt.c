#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

#include "../header/helper.h"
#include "../header/log.h"

uint32_t** files_data;



/**
 * @brief Object passed to worker threads
 * 
 * Holds all the data necessary for threads to work
 * The max amount of jobs that can be stored at any moment
 * The index in the job queue qhere the next job that needs to be done can be found
 * The list of jobs
 * The files data
 */
typedef struct{
    uint8_t* files_data;
    uint32_t files_size;
    int sockfd;
} thread_arg_t;

int create_and_bind_socket(struct sockaddr *listen_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /*
    int sf = fcntl(sockfd, F_GETFL);
    // Mark socket as non-blocking for accept() calls 
    fcntl(sockfd, F_SETFL, sf | O_NONBLOCK);
    */
    int res = bind(sockfd, listen_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt bind socket");
        return -1;
    }
    if ((listen(sockfd, 500)) != 0) {
        ERROR("Listen failed");
        exit(0);
    }
    return sockfd;
}

void* server_routine(int sock_desc, uint32_t file_size){
    //thread_arg_t* arg = (thread_arg_t*) varg;
    int sockfd = (int) sock_desc;
    unsigned file_number;
    unsigned key_size;
    int tread = recv(sockfd,&file_number, 4, 0);

    tread = recv(sockfd, &key_size, 4, 0);
    key_size = ntohl(key_size); // without, do not work

    uint32_t key[key_size*key_size];

    unsigned tot = key_size*key_size*sizeof(uint32_t);
    unsigned done = 0;
    while (done < tot) {
        tread = recv(sockfd, key, tot-done, 0);
        done += tread;
    }
    
    int nr = file_size / key_size;

    uint32_t* file = files_data[file_number % 1000];
    uint32_t* crypted = safe_malloc(file_size*file_size*sizeof(uint32_t*), __FILE__, __LINE__);
    long count = 0;
    
    for (int i = 0; i < nr; i++) {
        int vstart = i * key_size;
        for (int j = 0; j < nr; j++) {
            int hstart = j * key_size;

            for (int ln = 0; ln < key_size; ln++) {

                int aline = (vstart + ln) * file_size + hstart;
                for (int col = 0; col < key_size; col++) {
                    uint32_t r = key[ln*key_size + col];
                    int vline = (vstart+col) * file_size + hstart;

                    for (int k = 0; k < key_size; k++) {

                        crypted[aline + k] += file[vline + k]*r;
                    }
                }
            }
        }
    }
    //printf("count = %ld\n",count);

    uint8_t error = 0;
    send(sockfd, &error, 1, MSG_NOSIGNAL);
    unsigned sz = htonl(file_size*file_size*sizeof(uint32_t));
    send(sockfd, &sz, 4, MSG_NOSIGNAL);
    send(sockfd, crypted, sz, MSG_NOSIGNAL);

    

}

int main(int argc, char **argv) {
    srand(time(NULL));
    DEBUG("SERVER starting");
    int opt;

    // default params
    uint32_t files_size = 1024;
    uint16_t listen_port = (uint16_t) 2241; 

    while ((opt = getopt(argc, argv, "s:p:")) != -1) {
        switch (opt) {
        case 's':
            files_size = atoi(optarg);
            break;
        case 'p':
            listen_port = (uint16_t) atoi(optarg);
            break;
        default:
            ERROR("Invalid arg %c", opt);
            return -1;
        }
    }


    // GENERATE FILES -------------------------
    files_data = safe_malloc(1000*sizeof(uint32_t*), __FILE__, __LINE__);
    for (int i = 0; i < 1000; i++)
        files_data[i] = safe_malloc(files_size*files_size*sizeof(uint32_t*), __FILE__, __LINE__);

    

    // SETUP SOCKET --------------------------
    char *listen_ip = "0.0.0.0";

    struct sockaddr_in* listen_addr = build_ipv4_address(listen_ip, listen_port);
    socklen_t listen_addrlen = sizeof (struct sockaddr_in);

    int sockfd = create_and_bind_socket((struct sockaddr*) listen_addr, listen_addrlen);
    free(listen_addr);

    if(sockfd == -1){
        ERROR("Closing receiver, port is probably in use");
        return -2;
    }
    
    
    int client_sock;
    while (client_sock = accept(sockfd, (struct sockaddr*) listen_addr, &listen_addrlen)) {
        server_routine(client_sock, files_size);
    }
    
    
    free(files_data);
    
    free(listen_addr);

    return EXIT_SUCCESS;
}