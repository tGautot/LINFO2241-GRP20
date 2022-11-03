#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>



long current_time_millis(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t millis = tv.tv_sec*1000 + tv.tv_usec/1000;
    return millis;
}

int main(int argc, char const *argv[]) {

    
    int key_size = 2;
    int file_size = 8;
    int nr = file_size/key_size;

    uint32_t key[4] = {1,0,0,1};
    uint32_t file[64];
    uint32_t crypted[64];

    for (int i = 0; i < 64; i++) {
        file[i] = i;
        crypted[i] = 0;
    }


    for (int i = 0; i < nr; i++) {
        int vstart = i * key_size;
        for (int j = 0; j < nr; j++) {

            int hstart = j * key_size;

            for (int ln = 0; ln < key_size; ln++) {

                int aline = (vstart + ln) * file_size + hstart;
                for (int col = 0; col < key_size; col++) {

                    uint32_t r = key[ln*key_size + col];
                    //int sum = 0;
                    for (int k = 0; k < key_size; k++) {
                        //int vline = (vstart+k) * file_size + hstart;
                        int vline = (vstart+col) * file_size + hstart;
                        
                        crypted[aline + k] += r * file[vline + k];
                        
                        //sum += key[ln*key_size + k] * file[vline + col];
                    }
                    //crypted[aline + col] = sum;
                }
            }
        }
    }

    for (int i = 0; i < 64; i++) {
        printf("%d ",crypted[i]);
    }
    printf("\n");
    
    return 0;

    
}


/*
    printf("1\n");
    fflush(stdout);

    int n = 1600;

    char a[n][n];
    printf("1\n");

    char b[n][n];
    printf("1\n");
    fflush(stdout);
    char c[n][n];


    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            a[i][j] = 1;
            b[i][j] = 1;
            c[i][j] = 1;
        }
    }


    long start = current_time_millis();
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int sum = 0;
            for (int k = 0; k < n; k++)
                sum += a[i][k] * b[k][j];
            c[i][j] = sum;
        }
    }

    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            int r = a[i][k];
            for (int j = 0; j < n; j++)
                c[i][j] += r * b[k][j];
        }
    }

    long end = current_time_millis();

    printf("time = %ld\n",end-start);*/
