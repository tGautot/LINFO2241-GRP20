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
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int res = connect(sockfd, receiver_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt connect socket");
        return -1;
    }
    return sockfd;
}




int main(int argc, char **argv) {
    int opt;
    int key_size;
    int rate;
    int time_interval;

    while ((opt = getopt(argc, argv, "k:r:t")) != -1) {
        switch (opt) {
            case 'k':
                key_size = atoi(argv[1]);
                break;
            case 'r':
                rate = atoi(argv[2]);
                break;
            case 't':
                time_interval = atoi(argv[3]);
                break;
        }
    }

    printf("%d\n",1);
    char mystr[strlen(argv[7])];
    strcpy(mystr,argv[7]);
    printf("%d\n",2);
    
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

    struct client_message* message = malloc(sizeof(struct client_message)); 
    message->file_number = rand() % 1000;
    message->key_size = key_size;

    
    srand(time(NULL));
    for (int i = 0; i < key_size*key_size; i++){
        message->key[i] = (uint8_t) rand();
    }
    
    // Send message
    if (send(sockfd, message, sizeof(message), 0) < 0) {
        printf("Unable to send message\n");
        return -1;
    }

    struct server_message* response;
    // Receive server's response
    int a = recv(sockfd, response, sizeof(response), 0);
    if (a < 0) {
        printf("%d\n",a);
        printf("Error while receiving the response\n");
    }
    
    close(sockfd);

    return EXIT_SUCCESS;
}