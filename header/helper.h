#ifndef HELPER_H
#define HELPER_H

#include <stddef.h> 
#include <stdint.h> 
#include <unistd.h>
#include <stdio.h>  
#include <sys/time.h>

typedef struct client_message {
    uint32_t file_number;
    uint32_t key_size;
    uint8_t key[1000*1000];
} client_message_t;

typedef struct server_message {
    uint8_t error_code;
    uint32_t file_size;
    uint8_t encrpt_file[1000*1000];
} server_message_t;

void* safe_malloc(size_t nbytes, char* file, int line);
uint64_t current_time_millis();
struct sockaddr_in* build_ipv4_address(char* ip, uint16_t port);

#endif