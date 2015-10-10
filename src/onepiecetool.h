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
#ifndef ONEPIECE_TOOL_H
#define ONEPIECE_TOOL_H

#if defined __cplusplus
extern "C" {
#endif

#include "onepiece.h"

typedef struct resname_t {
    char                *name;
    struct resname_t    *next;
} resname_t;

void resname_free(resname_t *res);

typedef struct onepiecetool_option_t {
    char        *curr_id;       //current magnet info id
    char        *next_id;       //next magnet info id
    char        *app_name;      //app full filename
    char        *res_dir;       //resource directory, end with '/'
    resname_t   *res_list;      //resource file list
    char        *output_dir;    //output directory
    uint32_t    block_size;     //block size, default 8K
} onepiecetool_option_t;

onepiecetool_option_t* onepiecetool_option_malloc();
void onepiecetool_option_free(onepiecetool_option_t *option);

CRSYNCcode onepiecetool_perform(onepiecetool_option_t *option);

#if defined __cplusplus
    }
#endif

#endif // ONEPIECE_TOOL_H
