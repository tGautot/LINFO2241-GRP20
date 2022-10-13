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

#include "../header/helper.h"
#include "../header/log.h"


int* sockets_list;

int create_and_connect_socket(struct sockaddr *receiver_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int res = connect(sockfd, receiver_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt connect socket");
        return -1;
    }
    return sockfd;
}



int send_request(uint32_t key_size, char* mystr, char* receiver_ip, uint32_t receiver_port) {
    /*DEBUG("FINISHED PARSING");
    printf("%d\n",1);
    fflush(stdout);
    printf("%d\n",2);
    fflush(stdout);
    
    printf("%s\n",receiver_ip);
    

    printf("%d\n",receiver_port);*/

    struct sockaddr_in* receiver_addr = build_ipv4_address(receiver_ip, receiver_port);
    socklen_t receiver_addrlen = sizeof (struct sockaddr_in);


    int sockfd;
    sockfd = create_and_connect_socket((struct sockaddr*) receiver_addr, receiver_addrlen);
    
    if(sockfd < 0){
        ERROR("Couldn't connect to address, aborting send");
        return -2;
    }

    // Client is connected

    struct client_message* message = malloc(sizeof(client_message_t)); 
    srand(time(NULL));
    message->file_number = rand() % 1000;
    message->key_size = key_size;

    
    for (int i = 0; i < key_size*key_size; i++){
        message->key[i] = (uint8_t) rand();
    }
    
    DEBUG("Sengind query for file %d with key size %d (%u %u %u %u)", message->file_number, message->key_size,
        message->key[0], message->key[1], message->key[2], message->key[3]);
    // Send message
    // should use htonl but doesnt work with it
    message->file_number = (message->file_number);
    message->key_size = (message->key_size);
    DEBUG("Sending %ld bytes through network", sizeof(client_message_t));
    int err = send(sockfd, message, sizeof(client_message_t), 0);
    if (err < 0) {
        printf("Unable to send message %d\n", err);
        return -1;
    }

    return sockfd;
}

void recv_request(int sockfd) {
    server_message_t* response = malloc(sizeof(server_message_t));
    // Receive server's response
    DEBUG("Waiting for response");
    int a = recv(sockfd, response, sizeof(response), 0);
    if (a < 0) {
        ERROR("Error while receiving the response %d", a);
    }
    
    close(sockfd);

}



int main(int argc, char **argv) {
    int opt;
    int key_size;
    double rate;
    int time_interval;
    int n_request;
    int msg_send = 0;

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

    n_request = rate*time_interval;
    sockets_list = malloc(sizeof(int) * n_request);
    
    DEBUG("FINISHED PARSING");
    printf("%d\n",1);
    fflush(stdout);
    char mystr[strlen(argv[argc-1])];
    strcpy(mystr,argv[argc-1]);
    printf("%d\n",2);
    fflush(stdout);
    
    char *receiver_ip = strtok(mystr,":");
    printf("%s\n",receiver_ip);
    uint32_t receiver_port = atoi(strtok(NULL,":"));
    
    printf("%d\n",receiver_port);



    for (int k = 0; k < n_request; k++) {
        int t1 = current_time_millis();
        sockets_list[k] = send_request(key_size,mystr,receiver_ip,receiver_port);
        int t2 = current_time_millis();
        int time_request = t2-t1;

        if (1.0/rate > time_request)
            sleep((1.0/rate - time_request)*0.001);
    }
    
    for (int k = 0; k < n_request; k++) {
        recv_request(sockets_list[k]);
    }
            

    return EXIT_SUCCESS;
}




/*
void client_routine(uint32_t key_size, char* mystr, char* receiver_ip, uint32_t receiver_port) {
    DEBUG("FINISHED PARSING");
    printf("%d\n",1);
    fflush(stdout);
    printf("%d\n",2);
    fflush(stdout);
    
    printf("%s\n",receiver_ip);
    

    printf("%d\n",receiver_port);

    struct sockaddr_in* receiver_addr = build_ipv4_address(receiver_ip, receiver_port);
    socklen_t receiver_addrlen = sizeof (struct sockaddr_in);


    int sockfd;
    sockfd = create_and_connect_socket((struct sockaddr*) receiver_addr, receiver_addrlen);
    
    if(sockfd < 0){
        ERROR("Couldn't connect to address, aborting send");
        return -2;
    }

    // Client is connected

    struct client_message* message = malloc(sizeof(client_message_t)); 
    srand(time(NULL));
    message->file_number = rand() % 1000;
    message->key_size = key_size;

    
    for (int i = 0; i < key_size*key_size; i++){
        message->key[i] = (uint8_t) rand();
    }
    
    DEBUG("Sengind query for file %d with key size %d (%u %u %u %u)", message->file_number, message->key_size,
        message->key[0], message->key[1], message->key[2], message->key[3]);
    // Send message
    // should use htonl but doesnt work with it
    message->file_number = (message->file_number);
    message->key_size = (message->key_size);
    DEBUG("Sending %ld bytes through network", sizeof(client_message_t));
    int err = send(sockfd, message, sizeof(client_message_t), 0);
    if (err < 0) {
        printf("Unable to send message %d\n", err);
        return -1;
    }

    server_message_t* response = malloc(sizeof(server_message_t));
    // Receive server's response
    DEBUG("Waiting for response");
    int a = recv(sockfd, response, sizeof(response), 0);
    if (a < 0) {
        ERROR("Error while receiving the response %d", a);
    }
    
    close(sockfd);
}*/

