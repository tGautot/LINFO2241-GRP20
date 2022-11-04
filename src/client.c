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

    unsigned file_number = (rand() % 1000);

    uint32_t* key = safe_malloc(key_size*key_size*sizeof(uint32_t), __FILE__, __LINE__);
    
    for (int i = 0; i < key_size*key_size; i++){
        key[i] = (uint32_t) rand();
    }
    
    DEBUG("Sendind query for file %d with key size %d (%u %u %u %u)", file_number, key_size,
        key[0], key[1], key[2], key[3]);
    // Send message
    // should use htonl but doesnt work with it
    
    
    int err = send(sockfd, &file_number, 4, 0);
    if (err < 0) {
        //printf("Unable to send message %d\n", err);
        ERROR("Error while sending the file number");
    }

    int revkey = htonl(key_size);
    int err2 = send(sockfd, &revkey, 4, 0);
    if (err2 < 0) {
        //printf("Unable to send key %d\n", err);
        ERROR("Error while sending the key size");
    }
    
    int err3 = send(sockfd, key, sizeof(uint32_t)*key_size*key_size, 0);
    if (err3 < 0) 
        ERROR("Error while sending the key");


    // Receive server's response
    unsigned char error;
    DEBUG("Waiting for response");
    int a = recv(sockfd, &error, 1, 0);
    if (a < 0) {
        ERROR("Error while receiving the error code %d", a);
    }
    unsigned file_size;
    recv(sockfd, &file_size, 4, 0);
    file_size = ntohl(file_size);

    DEBUG("file_size = %d",file_size);
    
    /*
    if (file_size > 0) {
        DEBUG("0");

        long int left = file_size;
        char buffer[65536];
        while (left > 0) {
            DEBUG("1");

            unsigned b = left;
            if (b > 65536)
                b = 65536;
            left -= recv(sockfd, &buffer, b, 0);
        }
    }*/


    int64_t t2 = current_time_millis();
    pthread_mutex_lock(&lock);
    response_times[idx++] = t2-t1;
    pthread_mutex_unlock(&lock);

    
    free(key);
    close(sockfd);


}



int main(int argc, char **argv) {
    srand(time(NULL));

    int opt;
    int key_size;
    double rate;
    int time_interval;

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
    int n_request = rate*time_interval;


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

    DEBUG("FINISHED CLIENT");

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
