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

void* server_routine(int sockfd, uint32_t file_size){
    //thread_arg_t* arg = (thread_arg_t*) varg;
    
    unsigned file_number;
    unsigned key_size;
    int tread = recv(sockfd,&file_number, 4, 0);
    file_number = ntohl(file_number);
    tread = recv(sockfd, &key_size, 4, 0);
    key_size = ntohl(key_size); // without, do not work

    DEBUG("key_size = %d, file_number = %d\n",key_size, file_number);

    uint32_t key[key_size*key_size];

    unsigned tot = key_size*key_size*sizeof(uint32_t);
    unsigned done = 0;
    while (done < tot) {
        tread = recv(sockfd, key, tot-done, 0);
        done += tread;
    }
    
    int nr = file_size / key_size;

    uint32_t* file = files_data[file_number % 1000];
    uint32_t* crypted = safe_malloc(file_size*file_size*sizeof(uint32_t), __FILE__, __LINE__);
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

                    for (int k = 0; k < key_size; k+=8) {

                        crypted[aline + k] += file[vline + k]*r;
                        crypted[aline + k+1] += file[vline + k+1]*r;
                        crypted[aline + k+2] += file[vline + k+2]*r;
                        crypted[aline + k+3] += file[vline + k+3]*r;
                        crypted[aline + k+4] += file[vline + k+4]*r;
                        crypted[aline + k+5] += file[vline + k+5]*r;
                        crypted[aline + k+6] += file[vline + k+6]*r;
                        crypted[aline + k+7] += file[vline + k+7]*r;
                    }
                }
            }
        }
    }
    //printf("count = %ld\n",count);

    uint8_t error = 0;
    send(sockfd, &error, 1, MSG_NOSIGNAL);
    uint32_t sz = file_size*file_size*sizeof(uint32_t);
    //unsigned sz = (file_size*file_size*sizeof(uint32_t));
    uint32_t ntohlsz = ntohl(sz);
    send(sockfd, &ntohlsz, 4, MSG_NOSIGNAL);
    DEBUG("file_size = %d\n",sz);
    send(sockfd, crypted, sz, MSG_NOSIGNAL);
    free(crypted);
    close(sockfd);

}

int main(int argc, char **argv) {
    srand(time(NULL));
    DEBUG("SERVER starting");
    int opt;

    // default params
    int nthread = 1;
    uint32_t files_size = 1024;
    uint16_t listen_port = (uint16_t) 2241; 

    while ((opt = getopt(argc, argv, "j:s:p:")) != -1) {
        switch (opt) {
        case 'j':
            nthread = atoi(optarg);
            break;
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
        files_data[i] = safe_malloc(files_size*files_size*sizeof(uint32_t), __FILE__, __LINE__);
    for (int i = 0; i < files_size*files_size; i++)
        files_data[0][i] = i;
    

    // SETUP SOCKET --------------------------
    char *listen_ip = "0.0.0.0";

    struct sockaddr_in* listen_addr = build_ipv4_address(listen_ip, listen_port);
    socklen_t listen_addrlen = sizeof (struct sockaddr_in);

    int sockfd = create_and_bind_socket((struct sockaddr*) listen_addr, listen_addrlen);

    if(sockfd == -1){
        ERROR("Closing receiver, port is probably in use");
        return -2;
    }
    
    
    int client_sock;
    while (client_sock = accept(sockfd, (struct sockaddr*) listen_addr, &listen_addrlen)) {
        server_routine(client_sock, files_size);
    }
    
    for (int i = 0; i < 1000; i++)
        free(files_data[i]);
    free(files_data);
    
    free(listen_addr);

    return EXIT_SUCCESS;
}