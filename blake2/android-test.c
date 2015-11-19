#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "blake2.h"
#include "md5.h"

#define buflen 8*1024

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

int testMD5(const char *filename) {
    clock_t start = clock();
    FILE *f;
    int i, j;
    MD5_CTX ctx;
    uint8_t buf[buflen];
    uint8_t md5sum[16];

    if (!(f = fopen(filename, "rb"))) {
        printf("Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    MD5_Init(&ctx);

    while ((i = fread(buf, 1, buflen, f)) > 0)
        MD5_Update(&ctx, buf, i);

    fclose(f);
    MD5_Final(md5sum, &ctx);

    clock_t end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("md5 time = %f seconds\n", time);
    for (j = 0; j < 16; j++)
        printf("%02x", md5sum[j]);

    printf("\n");
}

int main( int argc, char **argv ) {
    const char *filename = "/sdcard/test.obb";
    tests(filename);
    testsp(filename);
    clock_t start = clock();
    uint8_t sum[BLAKE2S_OUTBYTES];
    blake2sp_file(filename, sum, BLAKE2S_OUTBYTES);
    clock_t end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("blake2sp_file time = %f seconds\n", time);
    for(int i=0; i<BLAKE2S_OUTBYTES; i++) {
        printf("%02x", sum[i]);
    }
    printf("\n");

    testMD5(filename);

    return 0;
}
