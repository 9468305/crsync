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
#include <stdio.h>

#include "crsync.h"

#if defined __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
#  define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif
/*
const int Default_blocksize = 2*1024; //2K

#define SSSIZE 16

PACK(
struct sum {
    uint16_t    a;
    uint16_t    b;
    uint8_t     s[SSSIZE];
});

PACK(
struct sumHead {
    uint32_t    file_sz;
    uint32_t    block_sz;
    uint8_t     file_Sum[SSSIZE];
});

struct sumTable {
    int weak;
    uint8_t strong[SSSIZE];
    int offset;
    UT_hash_handle hh;
};



void rsum_calc_block(const uint8_t *p, uint32_t start, uint32_t block_sz, struct sum *s)
{
    uint16_t a = 0;
    uint16_t b = 0;
    uint8_t *data = p+start;

    while(block_sz)
    {
        uint8_t c = *data++;
        a+=c;
        b+=block_sz*c;
        block_sz--;
    }
    s->a = a;
    s->b = b;
    memset(s->s, 1, SSSIZE);
}

uint32_t weak_calc_block(const uint8_t *p, uint32_t start, uint32_t block_sz)
{
    uint16_t a = 0;
    uint16_t b = 0;
    uint8_t  c = 0;

    while(block_sz)
    {
        c = p[start];
        a += c;
        b += block_sz*c;
        start++;
        block_sz--;
    }
    return (a & 0xff) | (b << 16);
}

uint32_t weak_calc_rolling(const uint8_t *p, uint32_t start, uint32_t block_sz, uint32_t weak)
{
    uint16_t a = weak & 0xff;
    uint16_t b = weak << 16;

    a -= p[start];
    b -= block_sz * p[start];
    a += p[start + block_sz];
    b += a;

    return (a & 0xff) | (b << 16);
}

void rsum_calc_rolling(const uint8_t *p, uint32_t start, uint32_t block_sz, struct sum *s)
{
    uint16_t a = s->a;
    uint16_t b = s->b;
    uint8_t *data = p+start;

    a -= *data;
    b -= block_sz * (*data);
    a += *(data + block_sz);
    b += a;

    s->a = a;
    s->b = b;
}

void strong_calc_block(const uint8_t *p, uint32_t start, uint32_t block_sz, uint8_t *strong)
{
    //TODO: MD4 MD5 BLAKE2s
    memset(strong, 0, SSSIZE);
}
*/
/*
struct rsum rcksum_calc_rsum_block(const unsigned char *data, size_t len) {
    unsigned short a = 0;
    unsigned short b = 0;

    while (len) {
        unsigned char c = *data++;
        a += c;
        b += len * c;
        len--;
    }
    struct rsum r = { a, b };
    return r;
}
*/



void sync_new_sig(const char* newFilename, const char* sigFilename)
{/*
    tpl_mmap_rec rec;

    int result = tpl_mmap_file(newFilename, &rec);
    if(!result)
    {
        rsum_meta_t meta;
        rsum_t *sumTable = NULL;
        rsum_t *sumItem = NULL;
        rsum_t *sumFind = NULL;

        meta.file_sz = rec.text_sz;
        meta.block_sz = Default_blocksize;
        memset(meta.file_sum, 0, SSSIZE);

        int blockCount = meta.file_sz / meta.block_sz;
        int i = 0;
        for(i = 0; i < blockCount; i++)
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
        tn = tpl_map("uuc#A(uuc#)", &meta.file_sz, &meta.block_sz, meta.file_sum, SSSIZE, &seq, &weak, &strong, SSSIZE);
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
        tpl_free(tn);


        struct sumHead sHead;
        tpl_bin tb;
        struct sum *sBody;
        int blockCount;
        int i;
        uint8_t *p;

        sHead.file_sz = rec.text_sz;
        sHead.block_sz = Default_blocksize;
        blockCount = sHead.file_sz / sHead.block_sz;
        tpl_bin_malloc(&tb, blockCount * sizeof(struct sum));
        sBody = (struct sum*)tb.addr;

        for(i = 0, p = (uint8_t*)rec.text; i < blockCount; i++)
        {
            rsum_calc_block(rec.text, i*sHead.block_sz, sHead.block_sz, &sBody[i]);
        }

        tpl_node *tn;
        tn = tpl_map("S(uuc#)B", &sHead, SSSIZE, &tb);
        tpl_pack(tn,0);

        tpl_dump(tn, TPL_FILE, newSigname);
        tpl_free(tn);

        tpl_bin_free(&tb);

        tpl_unmap_file(&rec);
    }*/
}

void sync_old_sig_delta(const char* oldFilename, const char* sigFilename, const char* deltaFilename)
{
/*
    struct sumHead sHead;
    tpl_bin tb;
    struct sum *sBody;

    tpl_node *tn;
    tn = tpl_map("S(uuc#)B", &sHead, SSSIZE, &tb);
    tpl_load(tn, TPL_FILE, sigFilename);
    tpl_unpack(tn, 0);
    tpl_free(tn);

    int blockCount = sHead.file_sz / sHead.block_sz;
    sBody = (struct sum*)tb.addr;
    //create hashtable
    struct sumTable *sumT = NULL;
    struct sumTable *sumItem;

    int i = 0;
    for(; i < blockCount ; i++)
    {
        sumItem = malloc(sizeof(struct sumTable));
        sumItem->weak = (sBody[i].a & 0xff | (sBody[i].b << 16));
        sumItem->offset = -1;
        memcpy(sumItem->strong, sBody[i].s, SSSIZE);
        HASH_ADD_INT( sumT, weak, sumItem );
    }



    tpl_mmap_rec rec;
    int result = tpl_mmap_file(oldFilename, &rec);
    if(! result)
    {
        int got = 0;

        //uint8_t *p = rec.text;
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
        }
        tpl_unmap_file(&rec);
        printf("blockCount %d, got %d\n", blockCount, got);
    }

    struct sumTable *current_user, *tmp;

    HASH_ITER(hh, sumT, current_user, tmp) {
        HASH_DEL(sumT,current_user);
        free(current_user);
      }

    free(tb.addr);*/
}

int main(void)
{
    const char* newFilename = "d:\\obb\\chapter0_14012.obb";
    const char* sigFilename = "14012.sig";
    const char* oldFilename = "d:\\obb\\chapter0_14009.obb";
    const char* deltaFilename = "14009_14012.delta";

    printf("crsync_hatch begin\n");

    crsync_hatch(newFilename, sigFilename);

    printf("crsync_hatch end\n");

    printf("crsync_match begin\n");
    crsync_match(oldFilename, sigFilename, deltaFilename);
    printf("crsync_match end\n");

    return 0;
}

#if defined __cplusplus
    }
#endif

