#ifndef HELPER_H
#define HELPER_H

#include <stddef.h> 
#include <stdint.h> 
#include <unistd.h>
#include <stdio.h>  
#include <sys/time.h>

void* safe_malloc(size_t nbytes, char* file, int line);
uint64_t current_time_millis();
struct sockaddr_in6* build_ipv6_address(char* ip, uint16_t port);


#endif