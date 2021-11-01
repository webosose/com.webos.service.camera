/*
 * Mobile Communication Company, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2017 by LG Electronics Inc.
 *
 * All rights reserved. No part of this work may be reproduced, stored in a
 * retrieval system, or transmitted by any means without prior written
 * Permission of LG Electronics Inc.
 */
#include "CameraSolutionManager.h"
#include "CameraSolution.h"

// System definitions
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


CameraSolution::CameraSolution(CameraSolutionManager* mgr)
    : m_manager(mgr),
      dump_framecnt(0),
      needDebugInfo(false)
{

}

CameraSolution::~CameraSolution() {

}
#if 0
void CameraSolution::dumpImageToFile(const void *image, int size, char* filename) {
    char fname[100] = {0,};
    int fd = -1;

    if (image == NULL) {
        LOGE(LOG_RELEASE,"dumpYUVImageToFile: image is null\n");
        return;
    }

#ifdef FEATURE_MTK
    if (mkdir("/data/vendor/camera/yuv", 0777) == -1 && errno != EEXIST)
#else
    if (mkdir("/sdcard/yuv", 0777) == -1 && errno != EEXIST)
#endif
    {
         LOGE(LOG_RELEASE,"Failed to create dump folder");
         return;
    }

    time_t now = time(NULL);
    struct tm *pnow = localtime(&now);
    if (pnow != NULL) {
#ifdef FEATURE_MTK
        snprintf(fname, sizeof(fname), "/data/vendor/camera/yuv/%d_%d_%d_%d_%s.nv21", pnow->tm_hour, pnow->tm_min, pnow->tm_sec, dump_framecnt, filename);
#else
        snprintf(fname, sizeof(fname), "/sdcard/yuv/%d_%d_%d_%d_%s.nv21", pnow->tm_hour, pnow->tm_min, pnow->tm_sec, dump_framecnt, filename);
#endif
    } else {
#ifdef FEATURE_MTK
        snprintf(fname, sizeof(fname), "/data/vendor/camera/yuv/DUMP_%s.nv21", filename);
#else
        snprintf(fname, sizeof(fname), "/sdcard/yuv/DUMP_%s.nv21", filename);
#endif
    }

    fd = open(fname, O_RDWR | O_CREAT, 0777);
    if (fd < 0) {
        LOGE(LOG_RELEASE ,"dumpYUVImageToFile: file open error\n");
        return;
    }
    LOGI(LOG_RELEASE, "[SolutionDump] %s", fname);
    dump_framecnt++;

    write(fd, image, size);
    close(fd);
}
#endif

