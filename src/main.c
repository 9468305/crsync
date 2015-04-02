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
#include <time.h>

#include "crsync.h"

#if defined __cplusplus
extern "C" {
#endif

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
    const char* newFilename = "base14012.obb";
    const char* sigFilename = "base14012.obb.sig";
    const char* oldFilename = "base14009.obb";
    const char* deltaFilename = "base14009.obb.delta";

    clock_t t;

    printf("crsync_hatch begin\n");
    t = clock();
    crsync_hatch(newFilename, sigFilename);
    t -= clock();
    printf("crsync_hatch end %ld\n", -t);

    printf("crsync_match begin\n");
    t = clock();
    crsync_match(oldFilename, sigFilename, deltaFilename);
    t -= clock();
    printf("crsync_match end %ld\n", -t);

    return 0;
}

#if defined __cplusplus
    }
#endif

