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

void* thread_routine(void *varg){
    thread_arg_t* arg = (thread_arg_t*) varg;

    uint32_t file_size = arg->files_size;
    uint32_t file_nbyte = file_size*file_size;
    // This is used to hold the data that is being computed so that other threads can keep using shared memory
    uint8_t* tmp_file = safe_malloc(file_nbyte, __FILE__, __LINE__);
    uint8_t* tmp_result = safe_malloc(file_nbyte, __FILE__, __LINE__);

    pid_t tid = pthread_self();
    DEBUG("Thread %d starting routine", tid);
    struct sockaddr* sa = malloc(sizeof(struct sockaddr*));
    client_message_t* job_msg = malloc(sizeof(client_message_t));
    server_message_t* rslt_msg = malloc(sizeof(server_message_t));
    while(true){
        
        socklen_t salen = sizeof(struct sockaddr*);
        // accept is MT-safe
        int client_sfd = accept(arg->sockfd, sa, &salen);
        if(client_sfd < 0) continue;
        DEBUG("Thread %d accpected connection", tid);
        recv(client_sfd, job_msg, sizeof(client_message_t), 0);
        DEBUG("Thead %d is starting a job", tid);
        int ks = job_msg->key_size;
        tmp_file = &arg->files_data[job_msg->file_number * file_nbyte];

        // Do encryption
        if(file_size % ks != 0){
            ERROR("Invalid key size %d stopping job", ks);
            continue;
        }
        int b = file_size / ks;

        for(int i = 0; i < file_nbyte; i++) tmp_result[i] = 0;

        int c, r;
        for(int base_i = 0; base_i < file_size; base_i += ks){
            for(int base_j = 0; base_j < file_size; base_j += ks){
                //int currb = (base_i/ks)*b+(base_j/ks); int totb = b*b;
                //DEBUG("Thread %d starting block %d/%d", tid, currb, totb );
                for(int trg_i = 0; trg_i < ks; trg_i++){
                    for(int trg_j = 0; trg_j < ks; trg_j++){
                        for(int offset = 0; offset < ks; offset++){
                            c = tmp_file[ (base_i + offset)*file_size + base_j + trg_j];
                            r = job_msg->key[ trg_i*job_msg->key_size + offset];
                            tmp_result[ (base_i + trg_i)*file_size + base_j + trg_j ] += r*c;
                        }
                    }
                }
            }
        }
        rslt_msg->error_code = 0; // TODO
        rslt_msg->file_size = arg->files_size;
        memcpy(rslt_msg->encrpt_file, tmp_file, file_nbyte);
        DEBUG("Thead %d finished a job", tid);
        send(client_sfd, rslt_msg, sizeof(server_message_t), 0);
        DEBUG("Thead %d sent response", tid);
        close(client_sfd);
    }
    free(sendmsg);
    free(recvmsg);
    free(sa);
    free(tmp_file);
    free(tmp_result);

}

int main(int argc, char **argv) {
    srand(time(NULL));
    DEBUG("SERVER starting");
    int opt;

    // default params
    uint32_t nthread = 4, files_size = 1024;
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
    uint8_t* files_data;
    files_data = safe_malloc(1000*files_size*files_size, __FILE__, __LINE__);
    for(int j = 0; j < 1000*files_size*files_size; j++){
        // Maybe could improve this by calling rand once for every 4 bytes
        files_data[j] = rand();
    }


    

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
    
    // SETUP THREADS -------------------------
    pthread_t threads[nthread];
    thread_arg_t* targ = safe_malloc(sizeof(thread_arg_t), __FILE__, __LINE__);
    targ->files_data = files_data;
    targ->files_size = files_size;
    targ->sockfd = sockfd;
    for(int i = 0; i < nthread; i++){
        pthread_create(&threads[i], NULL, thread_routine, targ);
    }

    for(int i = 0; i < nthread; i++){
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}