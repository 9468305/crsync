/*
The MIT License (MIT)

Copyright (c) 2015 chenqi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "crsync.h"
#include "uthash.h"
#include "tpl.h"
#include "blake2.h"

#define STRONG_SUM_SIZE 32  /*strong checksum size*/
#define Default_BLOCK_SIZE 2048 /*default block size to 2K*/

#define HATCH_TPLMAP_FORMAT "uuc#BA(uc#)"
#define MATCH_TPLMAP_FORMAT "uuc#BA(uc#ui)"

typedef struct rsum_meta_t {
    uint32_t    file_sz;                    /*file length*/
    uint32_t    block_sz;                   /*block length*/
    uint8_t     file_sum[STRONG_SUM_SIZE];  /*file strong sum*/
    tpl_bin     rest_tb;                    /*rest data*/
} rsum_meta_t;

typedef struct rsum_t {
    uint32_t        weak;                   /*first key*/
    uint32_t        seq;                    /*second key*/
    uint8_t         strong[STRONG_SUM_SIZE];/*value*/
    int32_t         offset;                 /*value*/
    struct rsum_t   *sub;                   /*sub HashTable*/
    UT_hash_handle  hh;
} rsum_t;

void rsum_weak_block(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak)
{
    uint32_t i = 0, a = 0, b = 0;
    const uint8_t *p = data + start;
    for ( ; i < block_sz - 4; i += 4) {
        b += 4 * (a + p[i]) + 3 * p[i + 1] + 2 * p[i + 2] + p[i + 3];
        a += p[i + 0] + p[i + 1] + p[i + 2] + p[i + 3];
    }
    for ( ; i < block_sz; i++) {
        a += p[i];
        b += a;
    }
    *weak = (a & 0xffff) | (b << 16);
}

void rsum_weak_rolling(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak)
{
    uint32_t a = *weak & 0xffff;
    uint32_t b = *weak >> 16;
    const uint8_t *p = data + start;

    a += p[block_sz] - p[0];
    b += a - block_sz * p[0];

    *weak = (a & 0xffff) | (b << 16);
}

void rsum_strong_block(const uint8_t *p, uint32_t start, uint32_t block_sz, uint8_t *strong)
{
    blake2b_state ctx;
    blake2b_init(&ctx, STRONG_SUM_SIZE);
    blake2b_update(&ctx, p+start, block_sz);
    blake2b_final(&ctx, (uint8_t *)strong, STRONG_SUM_SIZE);
}

void crsync_hatch(const char *newFilename, const char *sigFilename) {
    tpl_mmap_rec    rec = {-1, NULL, 0};
    rsum_meta_t     meta = {0, 0, "", {NULL, 0}};
    uint32_t        weak = 0, blocks=0, i=0;
    uint8_t         strong[STRONG_SUM_SIZE] = {""};
    tpl_node        *tn = NULL;
    int             result=0;

    result = tpl_mmap_file(newFilename, &rec);
    if (!result) {
        meta.file_sz = rec.text_sz;
        meta.block_sz = Default_BLOCK_SIZE;
        rsum_strong_block(rec.text, 0, rec.text_sz, meta.file_sum);
        blocks = meta.file_sz / meta.block_sz;
        i = meta.file_sz - blocks * meta.block_sz;
        if (i > 0) {
            tpl_bin_malloc(&meta.rest_tb, i);
            memcpy(meta.rest_tb.addr, rec.text + rec.text_sz - i, i);
        }

        tn = tpl_map(HATCH_TPLMAP_FORMAT,
                     &meta.file_sz,
                     &meta.block_sz,
                     meta.file_sum,
                     STRONG_SUM_SIZE,
                     &meta.rest_tb,
                     &weak,
                     &strong,
                     STRONG_SUM_SIZE);
        tpl_pack(tn, 0);

        for (i = 0; i < blocks; i++) {
            rsum_weak_block(rec.text, i*meta.block_sz, meta.block_sz, &weak);
            rsum_strong_block(rec.text, i*meta.block_sz, meta.block_sz, strong);
            tpl_pack(tn, 1);
        }

        tpl_dump(tn, TPL_FILE, sigFilename);
        tpl_free(tn);

        tpl_unmap_file(&rec);
    }
    tpl_bin_free(&meta.rest_tb);
}

void crsync_match(const char *oldFilename, const char *sigFilename, const char *deltaFilename) {
    tpl_mmap_rec    rec = {-1, NULL, 0};
    rsum_meta_t     meta = {0, 0, "", {NULL, 0}};
    uint32_t        weak = 0, blocks=0, i=0;
    uint8_t         strong[STRONG_SUM_SIZE] = {""};
    tpl_node        *tn = NULL;
    int             result=0;
    rsum_t          *sumTable = NULL, *sumItem = NULL, *sumIter = NULL, *sumTemp = NULL;

    /* load sig from file to hashtable */
    tn = tpl_map(HATCH_TPLMAP_FORMAT,
                 &meta.file_sz,
                 &meta.block_sz,
                 meta.file_sum,
                 STRONG_SUM_SIZE,
                 &meta.rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE);

    tpl_load(tn, TPL_FILE, sigFilename);
    tpl_unpack(tn, 0);

    blocks = meta.file_sz / meta.block_sz;

    for (i = 0; i < blocks; i++) {
        tpl_unpack(tn, 1);

        sumItem = malloc(sizeof(struct rsum_t));
        sumItem->weak = weak;
        sumItem->seq = i;
        memcpy(sumItem->strong, strong, STRONG_SUM_SIZE);
        sumItem->offset = -1;
        sumItem->sub = NULL;

        HASH_FIND_INT(sumTable, &weak, sumTemp);
        if (sumTemp == NULL) {
            HASH_ADD_INT( sumTable, weak, sumItem );
        } else {
            HASH_ADD_INT( sumTemp->sub, seq, sumItem );
        }
    }

    tpl_free(tn);

#if 0
    size_t size = HASH_OVERHEAD(hh, sumTable);
    printf("hashtable memory size=%ld\n", size);
#endif

    /* load old file and match with hashtable */
    result = tpl_mmap_file(oldFilename, &rec);
    if (!result) {
        /* TODO: check oldfile exist and size*/
        i = 0;
        rsum_weak_block(rec.text, i=0, meta.block_sz, &weak);

        while (1) {
            /*search match in table */
            HASH_FIND_INT( sumTable, &weak, sumItem );
            if(sumItem)
            {
                /* TODO: compare strong sum */
                rsum_strong_block(rec.text, i, meta.block_sz, strong);
                if (0 == memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
                    sumItem->offset = i;
                }
                HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                    if (0 == memcmp(strong, sumIter->strong, STRONG_SUM_SIZE)) {
                        sumIter->offset = i;
                    }
                }
            }

            if (i == rec.text_sz - meta.block_sz) {
                break;
            }

            rsum_weak_rolling(rec.text, i++, meta.block_sz, &weak);
        }
        tpl_unmap_file(&rec);
    }

    /* save hashtable to delta file */
    uint32_t seq;
    int32_t offset;
    tn = tpl_map(MATCH_TPLMAP_FORMAT,
                 &meta.file_sz,
                 &meta.block_sz,
                 meta.file_sum,
                 STRONG_SUM_SIZE,
                 &meta.rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE,
                 &seq,
                 &offset);
    tpl_pack(tn, 0);

    rsum_t *sumTemp2 = NULL;
    HASH_ITER(hh, sumTable, sumItem, sumTemp) {
        weak = sumItem->weak;
        memcpy(strong, sumItem->strong, STRONG_SUM_SIZE);
        seq = sumItem->seq;
        offset = sumItem->offset;
        tpl_pack(tn, 1);
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            weak = sumIter->weak;
            memcpy(strong, sumIter->strong, STRONG_SUM_SIZE);
            seq = sumIter->seq;
            offset = sumIter->offset;
            tpl_pack(tn, 1);
        }
    }

    tpl_dump(tn, TPL_FILE, deltaFilename);
    tpl_free(tn);

    HASH_ITER(hh, sumTable, sumItem, sumTemp) {
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            HASH_DEL(sumItem->sub, sumIter);
            free(sumIter);
        }
        HASH_DEL(sumTable, sumItem);
        free(sumItem);
    }

    tpl_bin_free(&meta.rest_tb);
}

void crsync_patch(const char *oldFilename, const char *deltaFilename, const char *newFilename) {
    /*load old file*/
    /*load new file*/
    /*load delta file*/
    /*patch to new file part */

    tpl_mmap_rec    recOld = {-1, NULL, 0};
    tpl_mmap_rec    recNew = {-1, NULL, 0};
    tpl_mmap_rec    recNewPart = {-1, NULL, 0};
    rsum_meta_t     meta = {0, 0, "", {NULL, 0}};
    uint32_t        weak = 0, blocks=0, i=0, seq=0;
    uint8_t         strong[STRONG_SUM_SIZE] = {""};
    tpl_node        *tn = NULL;
    int             result=0, offset = -1;
    rsum_t          *sumTable = NULL, *sumItem = NULL, *sumIter = NULL, *sumTemp = NULL;

    tpl_mmap_file(oldFilename, &recOld);
    tpl_mmap_file(newFilename, &recNew);

    tn = tpl_map(MATCH_TPLMAP_FORMAT,
                 &meta.file_sz,
                 &meta.block_sz,
                 meta.file_sum,
                 STRONG_SUM_SIZE,
                 &meta.rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE,
                 &seq,
                 &offset);

    tpl_load(tn, TPL_FILE, deltaFilename);
    tpl_unpack(tn, 0);

    blocks = meta.file_sz / meta.block_sz;

    for (i = 0; i < blocks; i++) {
        tpl_unpack(tn, 1);

        sumItem = malloc(sizeof(struct rsum_t));
        sumItem->weak = weak;
        sumItem->seq = seq;
        memcpy(sumItem->strong, strong, STRONG_SUM_SIZE);
        sumItem->offset = offset;
        sumItem->sub = NULL;

        HASH_FIND_INT(sumTable, &weak, sumTemp);
        if (sumTemp == NULL) {
            HASH_ADD_INT( sumTable, weak, sumItem );
        } else {
            HASH_ADD_INT( sumTemp->sub, seq, sumItem );
        }
    }

    tpl_free(tn);

    recNewPart.text_sz = meta.file_sz;
    tpl_mmap_file_output("new.part", &recNewPart);

    rsum_t *sumTemp2 = NULL;
    HASH_ITER(hh, sumTable, sumItem, sumTemp) {
        seq = sumItem->seq;
        offset = sumItem->offset;
        if(offset == -1) {
            memcpy(recNewPart.text + seq*meta.block_sz, recNew.text + seq*meta.block_sz, meta.block_sz);
        } else {
            memcpy(recNewPart.text + seq*meta.block_sz, recOld.text + offset, meta.block_sz);
        }
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            seq = sumIter->seq;
            offset = sumIter->offset;
            if(offset == -1) {
                memcpy(recNewPart.text + seq*meta.block_sz, recNew.text + seq*meta.block_sz, meta.block_sz);
            } else {
                memcpy(recNewPart.text + seq*meta.block_sz, recOld.text + offset, meta.block_sz);
            }
        }
    }

    if(meta.rest_tb.sz > 0) {
        memcpy(recNewPart.text + blocks * meta.block_sz, meta.rest_tb.addr, meta.rest_tb.sz);
    }

    tpl_unmap_file(&recNewPart);

    HASH_ITER(hh, sumTable, sumItem, sumTemp) {
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            HASH_DEL(sumItem->sub, sumIter);
            free(sumIter);
        }
        HASH_DEL(sumTable, sumItem);
        free(sumItem);
    }

    tpl_bin_free(&meta.rest_tb);

    tpl_unmap_file(&recNew);
    tpl_unmap_file(&recOld);
}
