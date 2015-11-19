#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "blake2.h"

static const uint32_t buflen = 16*024;

int tests(const char *filename) {
    clock_t start = clock();
    FILE *file = NULL;

    if ( (file = fopen(filename, "rb")) == NULL ) {
        printf("Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    uint8_t *buf = malloc(buflen);
    uint8_t *sum = malloc(BLAKE2S_OUTBYTES);
    blake2s_state S;
    blake2s_init( &S, BLAKE2S_OUTBYTES );
    size_t length = 0;
    while( 0 != (length = fread(buf, 1, buflen, file)) ) {
        blake2s_update( &S, ( const uint8_t * )buf, length );
    }
    blake2s_final( &S, sum, BLAKE2S_OUTBYTES );

    clock_t end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("blake2s time = %f seconds\n", time);
    for(int i=0; i<BLAKE2S_OUTBYTES; i++) {
        printf("%02x", sum[i]);
    }
    printf("\n");
    free(sum);
    free(buf);
    fclose(file);
    return 0;
}

int testsp(const char *filename) {
    clock_t start = clock();
    FILE *file = NULL;

    if ( (file = fopen(filename, "rb")) == NULL ) {
        printf("Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    uint8_t *buf = malloc(buflen);
    uint8_t *sum = malloc(BLAKE2S_OUTBYTES);
    blake2sp_state S;
    blake2sp_init( &S, BLAKE2S_OUTBYTES );
    size_t length = 0;
    while( 0 != (length = fread(buf, 1, buflen, file)) ) {
        blake2sp_update( &S, ( const uint8_t * )buf, length );
    }
    blake2sp_final( &S, sum, BLAKE2S_OUTBYTES );

    clock_t end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("blake2sp time = %f seconds\n", time);
    for(int i=0; i<BLAKE2S_OUTBYTES; i++) {
        printf("%02x", sum[i]);
    }
    printf("\n");
    free(sum);
    free(buf);
    fclose(file);
    return 0;
}


int main( int argc, char **argv ) {
    const char *filename = "/sdcard/test.obb";
    tests(filename);
    testsp(filename);
    //tests_mmap(filename);
    //testsp_mmap(filename);
}
