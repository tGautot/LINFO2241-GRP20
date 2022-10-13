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




int create_and_connect_socket(struct sockaddr *receiver_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int res = connect(sockfd, receiver_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt connect socket %d, err: %s", sockfd, strerror(errno));
        return -1;
    }
    return sockfd;
}




int main(int argc, char **argv) {
    int opt;
    int key_size;
    int rate;
    int time_interval;

    while ((opt = getopt(argc, argv, "k:r:t:")) != -1) {
        //DEBUG("New option %c val %s", opt, optarg);
        switch (opt) {
            case 'k':
                key_size = atoi(optarg);
                break;
            case 'r':
                rate = atoi(optarg);
                break;
            case 't':
                time_interval = atoi(optarg);
                break;
        }
    }
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

    return EXIT_SUCCESS;
}