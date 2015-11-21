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
#include "log.h"

#if CRSYNC_DEBUG
static log_t *log_head = NULL;

void log_append(const char *s) {
    log_t *log = calloc(sizeof(log_t), 1);
    log->content = strdup(s);
    LL_APPEND(log_head, log);
}

void logdump() {
    FILE *f = fopen(LOG_FILE, "a");
    if(f) {
        log_t *elt, *tmp;
        LL_FOREACH_SAFE(log_head,elt,tmp) {
            LL_DELETE(log_head,elt);
            fputs(elt->content, f);
            free(elt->content);
            free(elt);
        }
        log_head = NULL;
        fclose(f);
    }
}

#else
void logdump();
#endif //CRSYNC_DEBUG