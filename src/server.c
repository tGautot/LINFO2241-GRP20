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

#include "../header/helper.h"
#include "../header/log.h"


int create_and_bind_socket(struct sockaddr *listen_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    int res = bind(sockfd, listen_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt bind socket");
        return -1;
    }
    return sockfd;
}

int main(int argc, char **argv) {
    DEBUG("Receiver starting");
    int opt;

    char *listen_ip = "::";
    char *listen_port_err;
    uint16_t listen_port;

    listen_port = (uint16_t) 1359;

    struct sockaddr_in6* listen_addr = build_ipv6_address(listen_ip, listen_port);
    socklen_t listen_addrlen = sizeof (struct sockaddr_in6);

    int sockfd = create_and_bind_socket((struct sockaddr*) listen_addr, listen_addrlen);
    free(listen_addr);

    if(sockfd == -1){
        ERROR("Closing receiver, port is probably in use");
        return -2;
    }
    
    // Socket ready, start routine
    while(true){
        sleep(1);
        printf("[SRV]: Tourne\n");
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}