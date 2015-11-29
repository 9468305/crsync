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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"
#include "tpl.h"


static const char* s_hexTable[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
    "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
    "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
    "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
};

char* Util_hex_string(const unsigned char *in, const unsigned int inlen) {
    unsigned int outlen = inlen * 2;
    char *out = malloc(outlen+1);
    for(unsigned int i=0; i<inlen; ++i) {
        out[i*2] = s_hexTable[in[i]][0];
        out[i*2+1] = s_hexTable[in[i]][1];
    }
    out[outlen] = '\0';
    return out;
}

static unsigned char nibbleFromChar(char c) {
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 255;
}

unsigned char* Util_string_hex(const char *in) {
    unsigned char *retval;
    unsigned char *p;
    int len, i;

    len = strlen(in) / 2;
    retval = malloc(len+1);
    for(i=0, p = (unsigned char *) in; i<len; ++i) {
        retval[i] = (nibbleFromChar(*p) << 4) | nibbleFromChar(*(p+1));
        p += 2;
    }
    retval[len] = 0;
    return retval;
}

char* Util_strcat(const char* s1, const char *s2) {
    size_t len = strlen(s1) + strlen(s2) + 1;
    char *s = malloc( len );
    strcpy(s, s1);
    strcat(s, s2);
    s[len-1] = '\0';
    return s;
}

int Util_tplfile_check(const char *filename, const char* fmt) {
    char *f = tpl_peek(TPL_FILE, filename);
    int cmp = -1;
    if(f) {
        cmp = strncmp( f, fmt, strlen(fmt) );
        free(f);
    }
    return cmp;
}

int Util_copyfile(const char *src, const char *dst) {
    size_t size;
    FILE* s = fopen(src, "rb");
    FILE* d = fopen(dst, "wb");
    if(s && d) {
        static const int len = 8192;
        unsigned char *buf = malloc(len);
        while ((size = fread(buf, 1, len, s)) > 0) {
            fwrite(buf, 1, size, d);
        }
        free(buf);
    } else {
        fclose(s);
        fclose(d);
        return -1;
    }
    fclose(s);
    fclose(d);

    struct stat a, b;
    if(stat(src, &a) == 0 && stat(dst, &b) == 0) {
        if(a.st_size == b.st_size) {
            return 0;
        }
    }
    return -1;
}


void Util_rename(const char *src, const char *dst) {
    if(0 == Util_copyfile(src, dst)) {
        remove(dst);
    }
}
