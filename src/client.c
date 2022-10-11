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

#include "../header/helper.h"
#include "../header/log.h"

int create_and_connect_socket(struct sockaddr *receiver_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    int res = connect(sockfd, receiver_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt connect socket");
        return -1;
    }
    return sockfd;
}

int main(int argc, char **argv) {
    int opt;

    char *receiver_ip = "::1";
    char *receiver_port_err;
    uint16_t receiver_port;

    receiver_port = (uint16_t) 13;


    struct sockaddr_in6* receiver_addr = build_ipv6_address(receiver_ip, receiver_port);
    socklen_t receiver_addrlen = sizeof (struct sockaddr_in6);


    int sockfd;
    sockfd = create_and_connect_socket((struct sockaddr*) receiver_addr, receiver_addrlen);
    
    if(sockfd < 0){
        ERROR("Couldn't connect to address, aborting send");
        return -2;
    }

    // Client is connected
    while(true){
        sleep(1);
        printf("[CLT]: Tourne\n");
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}