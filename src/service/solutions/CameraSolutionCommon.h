/*
 * Mobile Communication Company, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2017 by LG Electronics Inc.
 *
 * All rights reserved. No part of this work may be reproduced, stored in a
 * retrieval system, or transmitted by any means without prior written
 * Permission of LG Electronics Inc.
 */

#ifndef __LGCAMERASOLUTIONCOMMON_H__
#define __LGCAMERASOLUTIONCOMMON_H__

/*standard library*/
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define LOG_TAG "LGCameraSolutionFW"

#define PACKAGE_NAME "com.lge.camerasolution.manager"
#define SOLUTION_FOLDER "/data/data/com.lge.camerasolution/"
#define KERNEL_FOLDER "/data/data/com.lge.camerasolution/clkernel/"
#define ETC_FOLDER "/data/data/com.lge.camerasolution/etc/"
#define DLC_FOLDER "/vendor/etc/camera/pie/"

#define LGCS_SUCCESS 1
#define LGCS_ERROR -1

#define PREVIEW_PARAMETER_MODE 0
#define SNAPSHOT_PARAMETER_MODE 1

#define MAX_FILM_EMULATOR_FILTER_TYPE 255
#define MAX_FACE_INFO_COUNT 10
#define MAX_ROI_COUNT 7

#define NUM_OF_PLANE 2
#define Y_PLANE 0
#define UV_PLANE 1

//LONG MAX_VALUE
#define INVALID_TIMESTAMP 9223372036854775807
#define INVALID_USAGE 99

extern "C" {

inline float MROUND(int num,int val) {if(val%num>num/2){val+=num-val%num;}else{val-=val%num;}return val;}


typedef struct {
    int left;
    int top;
    int width;
    int height;
} mrect_t;
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} rect_t;

typedef struct {
    struct plane {
        char *buffer;
        int stride;
    }plane[NUM_OF_PLANE];
    int width;
    int height;
    int stride;
    mrect_t focusROI;
    mrect_t AEROI;
    int faceNum;
    mrect_t faceROI[MAX_FACE_INFO_COUNT];
    int64_t timestamp;
    int64_t rollingShutterSkew;
    float real_gain;
    float exposure_time;
    float iso;
    int usage;
    char * buffer;

    int gridTransformEnable;
    int geometry[2];
    int transformDimension[2];
    float gridTransformArray[2*67*51];
}image_buf_t;

typedef struct {

    //Default
    int camera_id;

    float iso;
    float real_gain;
    float lux_index;
    float exposure_time;

    int face_count;
    mrect_t face_info[MAX_FACE_INFO_COUNT];

    int captureMode;
    int defaultBufferCount;
    int totalBufferCount; // Multiframe solution buffer count + outfocus buffer count
}parameter_t;


};

#endif // __LGCAMERASOLUTIONCOMMON_H__
