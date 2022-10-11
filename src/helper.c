#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "../header/helper.h"
#include "../header/log.h"




void* safe_malloc(size_t nbytes, char* file, int line){
    void* addr = malloc(nbytes);

    if(addr != NULL){
        return addr;
    }

    ERROR("%s, at line %d", file, line);
    ERROR("Could not allocated memory, exiting");
    exit(0);
}

uint64_t current_time_millis(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t millis = tv.tv_sec*1000 + tv.tv_usec/1000;
    return millis;
}

struct sockaddr_in* build_ipv4_address(char* ip, uint16_t port){
    struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    return addr;
}