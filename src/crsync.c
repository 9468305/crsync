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
/*
        for(i = 0; i < blocks; i++)
        {
            sumItem = malloc(sizeof(struct rsum_t));
            sumItem->weak = weak_calc_block(rec.text, i*meta.block_sz, meta.block_sz);
            strong_calc_block(rec.text, i*meta.block_sz, meta.block_sz, sumItem->strong);
            sumItem->seq = i;
            sumItem->offset = -1;
            sumItem->sub = NULL;

            HASH_FIND_INT(sumTable, &sumItem->weak, sumFind);
            if(sumFind)
            {
                HASH_ADD_INT(sumFind->sub, seq, sumItem);
            }
            else
            {
                HASH_ADD_INT( sumTable, weak, sumItem );
            }
        }

        uint32_t seq;
        uint32_t weak;
        uint32_t *strong;
        rsum_t *sum1, *sum2, *sumTemp1, *sumTemp2;
        tpl_node *tn;
        tn = tpl_map("uuc#A(uuc#)", &meta.file_sz, &meta.block_sz, meta.file_sum, STRONG_SUM_SIZE, &seq, &weak, &strong, STRONG_SUM_SIZE);
        tpl_pack(tn, 0);
        HASH_ITER(hh, sumTable, sum1, sumTemp1) {
            seq = sum1->seq;
            weak = sum1->weak;
            strong = sum1->strong;
            tpl_pack(tn, 1);
            HASH_ITER(hh, sum1->sub, sum2, sumTemp2) {
                seq = sum2->seq;
                weak = sum2->weak;
                strong = sum2->strong;
                tpl_pack(tn, 1);
                HASH_DEL(sum1->sub, sum2);
            }
            HASH_DEL(sumTable, sum1);
        }
        tpl_dump(tn, TPL_FILE, sigFilename);
        tpl_free(tn);*/

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
    //load sig from file to hashtable
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
//mmap old file
    result = tpl_mmap_file(oldFilename, &rec);
    if (!result)
    {
        //TODO: check oldfile exist and size
        i = 0;
        rsum_weak_block(rec.text, i=0, meta.block_sz, &weak);
int got = 0;
        while (1) {
            //search match in table
            HASH_FIND_INT( sumTable, &weak, sumItem );
            if(sumItem)
            {
                //TODO: compare strong sum
                rsum_strong_block(rec.text, i, meta.block_sz, strong);
                if (0 == memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
                    sumItem->offset = i;
                    //printf("match %d offset %d\n", sumItem->seq, i);
                    got++;
                }
                HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                    if (0 == memcmp(strong, sumIter->strong, STRONG_SUM_SIZE)) {
                        sumIter->offset = i;
                        //printf("match %d offset %d\n", sumIter->seq, i);
                        got++;
                    }
                }
            }

            if (i == rec.text_sz - meta.block_sz) {
                break;
            }

            rsum_weak_rolling(rec.text, i++, meta.block_sz, &weak);
        }
printf("got %d\n", got);
/*
        int got = 0;

        struct sum s;
        int start = 0;
        int end = rec.text_sz - sHead.block_sz;
        rsum_calc_block(rec.text, start, sHead.block_sz, &s);
        while(start < end)
        {
            rsum_calc_rolling(rec.text, start, sHead.block_sz, &s);
            int weak = ((s.a & 0xff) | (s.b << 16));
            struct sumTable *si = NULL;
            HASH_FIND_INT( sumT, &weak, si );
            if(si)
            {
                si->offset =start;
                got++;
            }
            start++;
        }*/
        tpl_unmap_file(&rec);
    }
/*
    struct sumTable *current_user, *tmp;

    HASH_ITER(hh, sumT, current_user, tmp) {
        HASH_DEL(sumT,current_user);
        free(current_user);
      }
*/


    //match and build delta
    //save delta to file
    tpl_bin_free(&meta.rest_tb);
}

void crsync_patch(const char *oldFilename, const char *deltaFilename, const char *newFilename) {

}
