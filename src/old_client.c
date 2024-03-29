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
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "../header/helper.h"
#include "../header/log.h"


pthread_t* threads;
int idx = 0;
pthread_mutex_t lock;
int64_t* response_times;

typedef struct {
    uint32_t key_size;
    char* mystr;
    uint16_t receiver_port;
    char* receiver_ip;
} arg_t;

//uint32_t key_size, char* mystr, char* receiver_ip, uint32_t receiver_port

int create_and_connect_socket(struct sockaddr *receiver_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int res = connect(sockfd, receiver_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt connect socket %d, err: %s", sockfd, strerror(errno));
        return -1;
    }
    return sockfd;
}






void* client_routine(void* thread_arg) {
    DEBUG("FINISHED PARSING");
    

    int64_t t1 = current_time_millis();

    arg_t* arg = (arg_t*) thread_arg;

    uint32_t key_size = arg->key_size;
    char* receiver_ip = arg->receiver_ip;
    uint16_t receiver_port = arg->receiver_port;
    char* mystr = arg->mystr;
    

    struct sockaddr_in* receiver_addr = build_ipv4_address(receiver_ip, receiver_port);
    socklen_t receiver_addrlen = sizeof (struct sockaddr_in);


    int sockfd;
    sockfd = create_and_connect_socket((struct sockaddr*) receiver_addr, receiver_addrlen);
    
    if(sockfd < 0){
        ERROR("Couldn't connect to address, aborting send");
        //return -2;
    }

    // Client is connected

    struct client_message* message = safe_malloc(sizeof(client_message_t), __FILE__, __LINE__); 
    message->file_number = rand() % 1000;
    message->key_size = key_size;
    uint8_t* key = safe_malloc(key_size*key_size, __FILE__, __LINE__);

    
    for (int i = 0; i < key_size*key_size; i++){
        key[i] = (uint8_t) rand();
    }
    
    DEBUG("Sengind query for file %d with key size %d (%u %u %u %u)", message->file_number, message->key_size,
        key[0], key[1], key[2], key[3]);
    // Send message
    // should use htonl but doesnt work with it
    message->file_number = (message->file_number);
    message->key_size = (message->key_size);
    DEBUG("Sending %ld bytes through network", sizeof(client_message_t));
    int err = send(sockfd, message, sizeof(client_message_t), 0);
    if (err < 0) {
        //printf("Unable to send message %d\n", err);
        ERROR("Error while sending the message");
    }
    int err2 = send(sockfd, key, key_size*key_size, 0);
    if (err2 < 0) {
        //printf("Unable to send key %d\n", err);
        ERROR("Error while sending the key");
    }

    server_message_t* response = safe_malloc(sizeof(server_message_t), __FILE__, __LINE__);
    // Receive server's response
    DEBUG("Waiting for response");
    int a = recv(sockfd, response, sizeof(response), 0);
    if (a < 0) {
        ERROR("Error while receiving the response %d", a);
    }
    int file_nbyte = response->file_size*response->file_size;
    uint8_t* crypt = safe_malloc(file_nbyte, __FILE__, __LINE__);
    int b = recv(sockfd, crypt, file_nbyte, 0);
    if (b < 0) {
        ERROR("Error while receiving the encrypted file %d", a);
    }

    int64_t t2 = current_time_millis();
    pthread_mutex_lock(&lock);
    response_times[idx++] = t2-t1;
    pthread_mutex_unlock(&lock);

    free(response);
    free(message);
    free(crypt);
    free(key);
    close(sockfd);

}



int main(int argc, char **argv) {
    srand(time(NULL));

    int opt;
    int key_size = 128;
    double rate = 10.0;
    int time_interval = 10;
    int n_request = rate*time_interval;

    while ((opt = getopt(argc, argv, "k:r:t:")) != -1) {
        //DEBUG("New option %c val %s", opt, optarg);
        switch (opt) {
            case 'k':
                key_size = atoi(optarg);
                break;
            case 'r':
                rate = (double)atoi(optarg);
                break;
            case 't':
                time_interval = atoi(optarg);
                break;
        }
    }

    if (pthread_mutex_init(&lock, NULL) != 0) {
        //printf("\n mutex init failed\n");
        return 1;
    }

    n_request = rate*time_interval;
    threads = safe_malloc(sizeof(pthread_t) * n_request, __FILE__, __LINE__);
    response_times = safe_malloc(sizeof(double) * n_request, __FILE__, __LINE__);
    
    DEBUG("FINISHED PARSING");
    
    char mystr[strlen(argv[argc-1])];
    strcpy(mystr,argv[argc-1]);
    
    
    char *receiver_ip = strtok(mystr,":");
    
    uint32_t receiver_port = atoi(strtok(NULL,":"));
    

    arg_t arg;
    arg.key_size = key_size;
    arg.mystr = mystr;
    arg.receiver_ip = receiver_ip;
    arg.receiver_port = receiver_port;

    int64_t start = current_time_millis();
    for (int k = 0; k < n_request; k++) {
        //printf("Connection %d\n",k);
        pthread_create(&threads[k], NULL, client_routine, &arg);
        usleep((1.0/rate)*1000000);
    }

    for (int k = 0; k < n_request; k++) {
        pthread_join(threads[k],NULL);
    }
    int64_t end = current_time_millis();
    int64_t total_time = end-start;

    pthread_mutex_destroy(&lock);

    FILE* data;

    data = fopen("data.txt","w");
    if (data == NULL) {
        //printf("Cant open the file");
        exit(1);
    }
    for (int i = 0; i < n_request; i++) {
        fprintf(data,"%ld\n",response_times[i]);
    }
    fprintf(data,"%ld",total_time);
    fclose(data);

    free(threads);
    free(response_times);
            

    return EXIT_SUCCESS;
}
