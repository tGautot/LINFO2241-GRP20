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
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

#include "../header/helper.h"
#include "../header/log.h"

typedef struct {
    struct sockaddr* clientaddr;
    uint32_t querry_id;
    int sfd;
} pending_querry_t;

/**
 * @brief Represents the job built from a querry
 * 
 * Holds the argument received through the querry:
 * The index of the file requested
 * The size of the key
 * The key itself
 * The id of the querry this job was built from
 */
typedef struct{
    uint32_t file_idx;
    uint32_t key_size;
    uint8_t* key_data;
    uint64_t querry_id;
} thread_job_data_t;

/**
 * @brief Result of a querry
 * 
 * Has the file (encrypted or not) and same job id as the query this data originated from
 */
typedef struct{
    uint8_t err;
    uint8_t* file;
    uint64_t querry_id;
} thread_job_result_t;

/**
 * @brief Object passed to worker threads
 * 
 * Holds all the data necessary for threads to work
 * The max amount of jobs that can be stored at any moment
 * The index in the job queue qhere the next job that needs to be done can be found
 * The list of jobs
 * The files data
 */
typedef struct{
    pthread_mutex_t mtx_get_job;
    pthread_mutex_t mtx_finish_job;
    uint32_t job_queue_size;
    int32_t next_job_idx;
    int32_t next_rslt_idx;
    thread_job_data_t* job_queue;
    thread_job_result_t* result_queue;
    uint8_t* files_data;
    uint32_t files_size;
} thread_arg_t;

int create_and_bind_socket(struct sockaddr *listen_addr, socklen_t addrlen){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sf = fcntl(sockfd, F_GETFL);
    // Mark socket as non-blocking for accept() calls 
    fcntl(sockfd, F_SETFL, sf | O_NONBLOCK);
    int res = bind(sockfd, listen_addr, addrlen);
    if(res != 0){
        ERROR("Couldnt bind socket");
        return -1;
    }
    if ((listen(sockfd, 500)) != 0) {
        ERROR("Listen failed");
        exit(0);
    }
    return sockfd;
}

int server_routine(int sockfd, thread_arg_t* targ){

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int max_poll_ms = 0;

    
    struct sockaddr_storage* addrstor = safe_malloc(sizeof(struct sockaddr_storage), __FILE__, __LINE__);
    struct sockaddr_in* rcvd_addr = safe_malloc(sizeof(struct sockaddr_in), __FILE__, __LINE__); 

    socklen_t addrstor_len = _SS_SIZE;
    uint32_t nw_in_len = sizeof(client_message_t);
    client_message_t rcvd_msg;
    server_message_t sent_msg;


    uint32_t nxt_querry_id = 0;
    uint32_t max_pending_querries = 64000; // Param;
    uint32_t nxt_available_idx = max_pending_querries -1;
    
    // Top value (at index nxt_available_idx) of this hold index in querries array to put next received querry
    uint32_t available_querry_indx[max_pending_querries]; 
    
    pending_querry_t* querries[max_pending_querries];
    for(int i = 0; i < max_pending_querries; i++){
        querries[i] = safe_malloc(sizeof(pending_querry_t), __FILE__, __LINE__);
        querries[i]->querry_id = -1; // id -1 means not a valid querry
        querries[i]->clientaddr = safe_malloc(sizeof(struct sockaddr_storage), __FILE__, __LINE__);;
        available_querry_indx[i] = max_pending_querries - 1 - i;
    }

    DEBUG("Routine loop starting");
    while(true){
        // Send results
        pthread_mutex_lock(&targ->mtx_finish_job);

        if(targ->next_rslt_idx >= 0){
            thread_job_result_t rslt = targ->result_queue[targ->next_rslt_idx];
            server_message_t* tosend = safe_malloc(sizeof(server_message_t), __FILE__, __LINE__);
            tosend->error_code = rslt.err;
            tosend->file_size = targ->files_size;
            memcpy(tosend->encrpt_file, rslt.file, targ->files_size * targ->files_size);
            free(rslt.file);
            uint32_t qidx = -1;
            for(int i = 0; i < max_pending_querries; i++){
                if(querries[i]->querry_id == rslt.querry_id){
                    qidx = i;
                    break;
                }
            }

            targ->next_rslt_idx--;
            nxt_available_idx++;
            available_querry_indx[nxt_available_idx] = qidx;
            if(qidx == -1){
                ERROR("Could not find matching querry with id %ld in list of pending querries, dropping result", rslt.querry_id);
            } else {
                DEBUG("Sending response of job %ld (querry index %d)", rslt.querry_id, qidx);
                send(querries[qidx]->sfd, tosend, sizeof(server_message_t), 0);
            }
            close(querries[qidx]->sfd);
        }

        pthread_mutex_unlock(&targ->mtx_finish_job);

        // Accept connection (up to 100 querries at once)
        for(int qi = 0; qi < 100; qi++){
            struct sockaddr_in cltaddr;
            socklen_t cltaddr_len;
            int qsfd = accept(sockfd, (struct sockaddr*) &cltaddr, &cltaddr_len);
            if(qsfd == -1) break;

            // Receive querry
            // This is not very well done, easily ddosable
            DEBUG("Received connection, waiting for message");
            nw_in_len = recv(qsfd, (void*) &rcvd_msg, nw_in_len, 0);

            if(nw_in_len < 0){
                ERROR("Couldnt read query from network");
                return -1;
            }

            DEBUG("Querry for file %d with keysize %d", rcvd_msg.file_number, rcvd_msg.key_size);
            if(rcvd_msg.key_size >= 4) DEBUG("Key starts with %u %u %u %u", rcvd_msg.key[0], rcvd_msg.key[1], rcvd_msg.key[2], rcvd_msg.key[3]);

            pthread_mutex_lock(&targ->mtx_get_job);
            
            targ->next_job_idx++;
            // Should use ntohl but it doesnt work with it
            targ->job_queue[targ->next_job_idx].file_idx = (rcvd_msg.file_number);
            targ->job_queue[targ->next_job_idx].key_data = malloc(rcvd_msg.key_size*rcvd_msg.key_size);
            memcpy(targ->job_queue[targ->next_job_idx].key_data, rcvd_msg.key, rcvd_msg.key_size*rcvd_msg.key_size);
            targ->job_queue[targ->next_job_idx].key_size = (rcvd_msg.key_size);
            
            nxt_querry_id++;
            targ->job_queue[targ->next_job_idx].querry_id = nxt_querry_id; 
            uint32_t qidx = available_querry_indx[nxt_available_idx];
            nxt_available_idx--;
            DEBUG("Job has id %d and is at index %d", nxt_querry_id, qidx);
            memcpy(querries[qidx]->clientaddr, (struct sockaddr*) &cltaddr, cltaddr_len);
            querries[qidx]->querry_id = nxt_querry_id;
            querries[qidx]->sfd = qsfd;

            pthread_mutex_unlock(&targ->mtx_get_job);
        }   
    }


}

void* thread_routine(void *varg){
    thread_arg_t* arg = (thread_arg_t*) varg;

    uint32_t file_size = arg->files_size;
    uint32_t file_nbyte = file_size*file_size;
    // This is used to hold the data that is being computed so that other threads can keep using shared memory
    uint8_t* tmp_file = safe_malloc(file_nbyte, __FILE__, __LINE__);
    uint8_t* tmp_result = safe_malloc(file_nbyte, __FILE__, __LINE__);
    thread_job_data_t job_data;
    thread_job_result_t job_result;
    pid_t tid = pthread_self();
    DEBUG("Thread %d starting routine", tid);
    while(true){
        // next_job_idx is shared but this is a simple read, no need to protect it
        pthread_mutex_lock(&arg->mtx_get_job);
        if(arg->next_job_idx < 0){ 
            pthread_mutex_unlock(&arg->mtx_get_job);
            continue;
        }
        DEBUG("Thread %d found a job (%ld)", tid, job_data.querry_id);
        // Copy data to relieve lock before starting computation
        job_data = arg->job_queue[arg->next_job_idx];
        memcpy(tmp_file, &arg->files_data[job_data.file_idx*file_nbyte], file_nbyte);
        arg->next_job_idx--;
        pthread_mutex_unlock(&arg->mtx_get_job);

        // Clean up for next result (I hate memset)
        for(int i = 0; i < file_nbyte; i++){
            tmp_result[i] = 0;
        }

        // Do encryption
        int ks = job_data.key_size;
        if(file_size % ks != 0){
            // [TODO] send error message
            continue;
        }
        int b = file_size / ks;

        int c, r;
        for(int base_i = 0; base_i < file_size; base_i += ks){
            for(int base_j = 0; base_j < file_nbyte; base_j += ks){
                for(int trg_i = 0; trg_i < ks; trg_i++){
                    for(int trg_j = 0; trg_j < ks; trg_j++){
                        for(int offset = 0; offset < ks; offset++){
                            c = tmp_file[ (base_i + offset)*file_size + base_j + trg_j];
                            r = job_data.key_data[ trg_i*job_data.key_size + offset];
                            tmp_result[ (base_i + trg_i)*file_size + base_j + trg_j ] += r*c;
                        }
                    }
                }
            }
        }
        free(job_data.key_data);

        pthread_mutex_lock(&arg->mtx_finish_job);

        DEBUG("Thread %d finished its job (%ld)", tid, job_data.querry_id);

        arg->next_rslt_idx++;
        int rslt_id = arg->next_rslt_idx;
        memcpy(arg->result_queue[rslt_id].file, tmp_result, file_nbyte);
        arg->result_queue[rslt_id].querry_id = job_data.querry_id; 

        pthread_mutex_unlock(&arg->mtx_finish_job);
    }

}

int main(int argc, char **argv) {
    srand(time(NULL));
    DEBUG("SERVER starting");
    int opt;

    // default params
    uint32_t nthread = 4, files_size = 1024;
    uint16_t listen_port = (uint16_t) 2241; 

    while ((opt = getopt(argc, argv, "h:s:p")) != -1) {
        switch (opt) {
        case 'h':
            nthread = atoi(optarg);
            break;
        case 's':
            files_size = atoi(optarg);
            break;
        case 'p':
            listen_port = (uint16_t) atoi(optarg);
            break;
        default:
            ERROR("Invalid arg %c", opt);
            return -1;
        }
    }


    // GENERATE FILES -------------------------
    uint8_t* files_data;
    files_data = safe_malloc(1000*files_size*files_size, __FILE__, __LINE__);
    for(int j = 0; j < 1000*files_size*files_size; j++){
        // Maybe could improve this by calling rand once for every 4 bytes
        files_data[j] = rand();
    }


    // SETUP THREADS -------------------------
    pthread_t threads[nthread];
    uint32_t max_jobs = 1024; // [PARAM] this can be changed
    thread_arg_t* targ = safe_malloc(sizeof(thread_arg_t), __FILE__, __LINE__);
    targ->job_queue_size = max_jobs;
    targ->next_job_idx = -1;
    targ->next_rslt_idx = -1;
    targ->files_data = files_data;
    targ->job_queue = safe_malloc(max_jobs * sizeof(thread_job_data_t), __FILE__, __LINE__);
    targ->result_queue = safe_malloc(max_jobs * sizeof(thread_job_result_t), __FILE__, __LINE__);
    pthread_mutex_init(&targ->mtx_get_job, NULL);
    pthread_mutex_init(&targ->mtx_finish_job, NULL);
    for(int i = 0; i < nthread; i++){
        pthread_create(&threads[i], NULL, thread_routine, targ);
    }

    // SETUP SOCKET --------------------------
    char *listen_ip = "0.0.0.0";

    struct sockaddr_in* listen_addr = build_ipv4_address(listen_ip, listen_port);
    socklen_t listen_addrlen = sizeof (struct sockaddr_in);

    int sockfd = create_and_bind_socket((struct sockaddr*) listen_addr, listen_addrlen);
    free(listen_addr);

    if(sockfd == -1){
        ERROR("Closing receiver, port is probably in use");
        return -2;
    }
    
    // Socket ready, start routine
    server_routine(sockfd, targ);

    return EXIT_SUCCESS;
}