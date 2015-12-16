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
#if defined __cplusplus
extern "C" {
#endif

#include <sys/stat.h>
#include <dirent.h>

#include "log.h"
#include "crsync.h"
#include "http.h"
#include "helper.h"
#include "magnet.h"
#include "util.h"
#include "iniparser.h"
#include "utlist.h"

static const char *cmd_digest = "digest";
static const char *cmd_bulkDigest = "bulkDigest";
static const char *cmd_diff = "diff";
static const char *cmd_patch = "patch";
static const char *cmd_update = "update";
static const char *cmd_helper = "helper";
static const char *cmd_bulkHelper = "bulkHelper";

static void showUsage() {
    printf( "crsync Usage:\n"
            "crsync [Operation] [Parameters]\n"
            "Operation:\n"
            "    digest       : generate digest file\n"
            "    bulkDigest   : easy way to generate bulk-Files' digest and fdi(fileDigestIndex)\n"
            "    diff         : diff src-File with target-File to dst-File\n"
            "    patch        : not implement\n"
            "    update       : update src-File with target-File to dst-File\n"
            "    helper       : easy way to update src-File itself(download/diff/patch)\n"
            "    bulkHelper   : easy way to update bulk-Files(download/diff/patch)\n");
}

static void showUsage_digest() {
    printf( "digest Usage:\n"
            "crsync digest srcFilename dstFilename blockSize\n");
}

int main_digest(int argc, char **argv) {
    if(argc != 5) {
        showUsage_digest();
        return -1;
    }
    int c = 0;
    c++; //crsync.exe
    c++; //digest
    const char *srcFilename = argv[c++];
    const char *dstFilename = argv[c++];
    uint32_t blockSize = atoi(argv[c++]) * 1024;

    CRScode code = crs_perform_digest(srcFilename, dstFilename, blockSize);
    return code;
}

static void cleanDir(const char *dir) {
    LOGI("clean up %s\n", dir);
    DIR *dirp = opendir(dir);
    if(dirp) {
        struct dirent *direntp = NULL;
        while ((direntp = readdir(dirp)) != NULL) {
            if(0 == strcmp(direntp->d_name, ".") ||
               0 == strcmp(direntp->d_name, "..")) {
                continue;
            }
            LOGD("remove %s\n", direntp->d_name);
            char *f = Util_strcat(dir, direntp->d_name);
            remove(f);
            free(f);
        }
        closedir(dirp);
    } else {
#ifndef _WIN32
        mkdir(dir, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
#else
        mkdir(dir);
#endif
    }
}

static void showUsage_bulkDigest() {
    printf("bulkDigest Usage:\n"
           "crsync bulkDigest iniFile\n");
}

int main_bulkDigest(int argc, char **argv) {
    if(argc != 3) {
        showUsage_bulkDigest();
        return -1;
    }
    int c = 0;
    c++; //crsync.exe
    c++; //bulkDigest
    const char *iniFilename = argv[c++];

    dictionary* dic = iniparser_load(iniFilename);
    if(!dic) {
        printf("%s load fail\n", iniFilename);
        return -1;
    }

    unsigned char hash[CRS_STRONG_DIGEST_SIZE];
    magnet_t* m = magnet_malloc();

    do {
        int sectionNum = iniparser_getnsec(dic);
        if(sectionNum <= 0) break;
        const char *outputDir = iniparser_getstring(dic, "global:outputDir", NULL);
        if(!outputDir) break;
        cleanDir(outputDir);
        const char *currVersion = iniparser_getstring(dic, "global:currVersion", NULL);
        if(!currVersion) break;
        const char *nextVersion = iniparser_getstring(dic, "global:nextVersion", NULL);
        if(!nextVersion) break;
        unsigned int blockSize = iniparser_getint(dic, "global:blockSize", 16);
        blockSize *= 1024;

        m->currVersion = strdup(currVersion);
        m->nextVersion = strdup(nextVersion);

        for(int i=1; i<sectionNum; ++i) {
            sum_t *sum = sum_malloc();
            const char *secname = iniparser_getsecname(dic, i);

            char *dirKey = Util_strcat(secname, ":dir");
            const char *dirValue = iniparser_getstring(dic, dirKey, NULL);
            free(dirKey);

            char *nameKey = Util_strcat(secname, ":name");
            const char *nameValue = iniparser_getstring(dic, nameKey, NULL);
            free(nameKey);

            sum->name = strdup(nameValue);

            char *srcFilename = Util_strcat(dirValue, nameValue);
            struct stat st;
            if(stat(srcFilename, &st) == 0) {
                sum->size = st.st_size;
            } else {
                LOGE("%s not exist\n", srcFilename);
                sum_free(sum);
                break;
            }

            Digest_CalcStrong_File(srcFilename, hash);
            memcpy(sum->digest, hash, CRS_STRONG_DIGEST_SIZE);
            LL_APPEND(m->file, sum);
            char *hashString = Util_hex_string(hash, CRS_STRONG_DIGEST_SIZE);
            char *dstFilename = Util_strcat(outputDir, hashString);
            char *digestFilename = Util_strcat(dstFilename, DIGEST_EXT);

            if(CRS_OK != crs_perform_digest(srcFilename, digestFilename, blockSize)) {
                LOGE("crs_perform_digest fail\n");
            }
            if(0 != Util_filecpy(srcFilename, dstFilename)) {
                LOGE("copy file fail\n");
            }

            free(srcFilename);
            free(dstFilename);
            free(digestFilename);
            free(hashString);
        }//end of for()

        char mfile[512];
        snprintf(mfile, 512, "%s%s%s", outputDir, currVersion, MAGNET_EXT);
        Magnet_Save(m, mfile);
    } while(0);

    iniparser_freedict(dic);
    magnet_free(m);
    return 0;
}

static void showUsage_diff() {
    printf( "diff Usage:\n"
            "crsync diff srcFilename dstFilename url\n");
}

int main_diff(int argc, char **argv) {
    if(argc != 5) {
        showUsage_diff();
        return -1;
    }
    int c = 0;
    c++; //crsync.exe
    c++; //diff
    const char *srcFilename = argv[c++];
    const char *dstFilename = argv[c++];
    const char *digestUrl = argv[c++];

    CRScode code = HTTP_global_init();
    if(code != CRS_OK) {
        LOGE("end %d\n", CRS_INIT_ERROR);
        return CRS_INIT_ERROR;
    }

    fileDigest_t *fd = fileDigest_malloc();
    diffResult_t *dr = diffResult_malloc();
    code = crs_perform_diff(srcFilename, dstFilename, digestUrl, fd, dr);
    diffResult_dump(dr);
    fileDigest_free(fd);
    diffResult_free(dr);
    HTTP_global_cleanup();
    return code;
}

static void showUsage_patch() {
    printf( "patch Usage:\n"
            "crsync patch srcFilename dstFilename url\n");
}

int main_patch(int argc, char **argv) {
    (void)argc;
    (void)argv;
    showUsage_patch();
    return -1;
}

static void showUsage_update() {
    printf( "update Usage:\n"
            "crsync update srcFilename dstFilename digestUrl url\n");
}

int main_update(int argc, char **argv) {
    if(argc != 6) {
        showUsage_update();
        return -1;
    }
    int c = 0;
    c++; //crsync.exe
    c++; //update
    const char *srcFilename = argv[c++];
    const char *dstFilename = argv[c++];
    const char *digestUrl = argv[c++];
    const char *url = argv[c++];

    CRScode code = HTTP_global_init();
    if(code != CRS_OK) {
        LOGE("end %d\n", CRS_INIT_ERROR);
        return CRS_INIT_ERROR;
    }

    code = crs_perform_update(srcFilename, dstFilename, digestUrl, url);

    HTTP_global_cleanup();
    return code;
}

static void showUsage_helper() {
    printf( "helper Usage:\n"
            "crsync helper FileDir baseUrl filename fileSize fileDigestString\n");
}

int main_helper(int argc, char **argv) {
    if(argc != 7) {
        showUsage_helper();
        return -1;
    }
    int c = 0;
    c++; //crsync.exe
    c++; //helper
    char *fileDir = argv[c++];
    char *baseUrl = argv[c++];
    char *fileName = argv[c++];
    const unsigned int fileSize = atoi(argv[c++]);
    char *fileDigestString = argv[c++];

    CRScode code = HTTP_global_init();
    if(code != CRS_OK) {
        LOGE("end %d\n", CRS_INIT_ERROR);
        return CRS_INIT_ERROR;
    }

    helper_t *h = helper_malloc();
    h->fileDir = fileDir;
    h->baseUrl = baseUrl;
    h->fileName = strdup(fileName);
    h->fileSize = fileSize;
    unsigned char *fileDigest = Util_string_hex(fileDigestString);
    memcpy( h->fileDigest, fileDigest, CRS_STRONG_DIGEST_SIZE);
    free(fileDigest);

    code = helper_perform_diff(h);
    if(code == CRS_OK) {
        code = helper_perform_patch(h);
    }

    helper_free(h);

    HTTP_global_cleanup();

    return code;
}

static void showUsage_bulkHelper() {
    printf( "bulkHelper Usage:\n"
            "crsync bulkHelper bulkFile\n");
}

bulkHelper_t* load_bulkFile(const char *bulkFile) {
    if(!bulkFile) {
        return NULL;
    }

    dictionary* dic = iniparser_load(bulkFile);
    if(!dic) {
        return NULL;
    }

    //TODO:

    iniparser_freedict(dic);

    return NULL;
}

int main_bulkHelper(int argc, char **argv) {
    if(argc != 3) {
        showUsage_bulkHelper();
        return -1;
    }
    int c = 0;
    c++; //crsync.exe
    c++; //bulkHelper
    char *bulkFile = argv[c++];
    bulkHelper_t *bh = load_bulkFile(bulkFile);
    if(!bh) {
        LOGE("end bulkFile format error\n");
        return -1;
    }

    CRScode code = HTTP_global_init();
    if(code != CRS_OK) {
        LOGE("end %d\n", CRS_INIT_ERROR);
        return CRS_INIT_ERROR;
    }

    code = bulkHelper_perform_diff(bh);

    //TODO: check here logic right ?
    if(code == CRS_OK) {
        code = bulkHelper_perform_patch(bh, (bh->latestVersion) ? 1 : 0 );
    }

    bulkHelper_free(bh);

    HTTP_global_cleanup();

    return code;
}

int main(int argc, char **argv) {
    log_open();
    for(int i=0; i<argc; i++) {
        LOGI("argv %d %s\n", i, argv[i]);
    }

    if(argc <= 2) {
        showUsage();
        log_close();
        return -1;
    }

    if(0 == strncmp(argv[1], cmd_digest, strlen(cmd_digest))) {
        return main_digest(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_bulkDigest, strlen(cmd_bulkDigest))) {
        return main_bulkDigest(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_diff, strlen(cmd_diff))) {
        return main_diff(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_patch, strlen(cmd_patch))) {
        return main_patch(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_update, strlen(cmd_update))) {
        return main_update(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_helper, strlen(cmd_helper))) {
        return main_helper(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_bulkHelper, strlen(cmd_bulkHelper))) {
        return main_bulkHelper(argc, argv);
    } else {
        showUsage();
    }
    log_close();
    return -1;
}

int crs_callback_patch(const char *basename, const unsigned int bytes, const int isComplete, const int immediate) {
    (void)immediate;
    fprintf(stdout, "crsync_progress %s %d %d\n", basename, bytes, isComplete);
    return 0;
}

void crs_callback_diff(const char *basename, const unsigned int bytes, const int isComplete, const int isNew) {
    fprintf(stdout, "crs_onDiff %s %d %d %d\n", basename, bytes, isComplete, isNew);
}

#if defined __cplusplus
}
#endif
