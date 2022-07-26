/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    AutoContrast.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  AutoContrast
 *
 */

#include <math.h>
#include <sys/time.h>

#include "auto_contrast.hpp"
#include "camera_types.h"
#define LOG_TAG "AutoContrast"
void brightnessEnhancement(unsigned char* inputY, unsigned char* inputUV,
                            int width, int height, int stride, int frameSize,
                            int minY, int maxY, int enhanceLevel);
void contrastEnhancement(unsigned char* inputY, unsigned char* inputUV,
                            int width, int height, int stride, int frameSize,
                            int minY, int maxY, int enhanceLevel);
int dumpFrame(unsigned char* inputY, unsigned char* inputUV,
                int width, int height, int stride, int frameSize,
                char* filename, char* filepath);

AutoContrast::AutoContrast(void)
{
    PMLOG_INFO(LOG_TAG, "");
    solutionProperty_ = Property(LG_SOLUTION_PREVIEW |
                                 LG_SOLUTION_VIDEO |
                                 LG_SOLUTION_SNAPSHOT);
}

AutoContrast::~AutoContrast(void)
{
    PMLOG_INFO(LOG_TAG, "");
    setEnableValue(false);
}

std::string AutoContrast::getSolutionStr(void)
{
    return SOLUTION_AUTOCONTRAST;
}

void AutoContrast::processForSnapshot(buffer_t inBuf)
{
    doAutoContrastProcessing(inBuf);
}

void AutoContrast::processForPreview(buffer_t inBuf)
{
    doAutoContrastProcessing(inBuf);
}

void AutoContrast::doAutoContrastProcessing(buffer_t inBuf)
{
    PMLOG_INFO(LOG_TAG, ">");
    //AutoContrast is only working on YUYV format currently
    if(streamFormat_.pixel_format != CAMERA_PIXEL_FORMAT_YUYV)
        return;

    int width     = streamFormat_.stream_width;
    int height    = streamFormat_.stream_height;
    int stride    = streamFormat_.stream_width;
    int scanline  = streamFormat_.stream_height;
    int frameSize = streamFormat_.buffer_size;
    int minY = 30;
    int maxY = 210;

    uint8_t *Yimage  = (unsigned char*)inBuf.start;
    uint8_t *UVimage = (unsigned char*)inBuf.start + (stride * scanline);
    PMLOG_INFO(LOG_TAG, "width(%d) height(%d) stride(%d) frameSize(%d) pixel_format(%d)",
                width, height, stride, frameSize, streamFormat_.pixel_format);

    contrastEnhancement(Yimage, UVimage, width, height, stride, frameSize, minY, maxY, 2.0f);

    if (0){
        PMLOG_INFO(LOG_TAG, "after AutoContrast_Execute error[%d]", -1/*retval*/);
    } else {
        PMLOG_INFO(LOG_TAG, "after AutoContrast works on snapshot");
    }

    PMLOG_INFO(LOG_TAG, "<");
}

void AutoContrast::release()
{
    PMLOG_INFO(LOG_TAG,  "");
}

void brightnessEnhancement(unsigned char* inputY, unsigned char* inputUV,
                           int width, int height, int stride, int frameSize,
                           int minY, int maxY, int enhanceLevel)
{
    PMLOG_INFO(LOG_TAG, ">");

    int range = abs(maxY - minY);
    int curveLUT[256];
    int brightnessLUT[256];
    int SaturationLUT[256];
    memset(curveLUT, 0, sizeof(curveLUT));
    memset(brightnessLUT, 0, sizeof(brightnessLUT));
    memset(SaturationLUT, 0, sizeof(SaturationLUT));
    for (int i = 0 ; i < 256 ; i++){
        curveLUT[i] = brightnessLUT[i] = SaturationLUT[i] = i;
    }

    float m_enhanceLevel = 1.0f + (float)(abs(enhanceLevel))/100.0f;

    for (int k = 0 ; k <= range/2 ; k++){
        curveLUT[k] = abs(k - (int)round((float)range-(float)range*(float)(pow(double(range-k)/(double)range,(double)m_enhanceLevel))));
        curveLUT[range - k] = curveLUT[k];
    }

    //for(int k = 0 ; k < range/2 ; k++){
    //    curveLUT[range - k] = curveLUT[k];
    //}

    for (int k = 0 ; k <= range ; k++){
        //curveLUT[k] = abs(k - (int)round((float)range-(float)range*(float)(pow(double(range-k)/(double)range,(double)mContrastLevelForTextMode))));
        int LUT_value = CLAMP(k - curveLUT[k], 0, range);
        if(enhanceLevel > 0){
            if(abs(LUT_value - k) > 200)
                brightnessLUT[k + minY] = k + minY;
            else
                brightnessLUT[k + minY] = CLAMP(k + minY + curveLUT[k], minY, maxY);
        }else{
            if(abs(LUT_value - k) > 200)
                brightnessLUT[k + minY] = k + minY;
            else
                brightnessLUT[k + minY] = CLAMP(k + minY - curveLUT[k], minY, maxY);
        }
    }

    for (int y = 0; y < height; y ++) {
        for (int x = 0; x < width; x ++) {
            inputY[y * stride + x] = brightnessLUT[inputY[y * stride + x]];
        }
    }

    float saturation_level = 1.0f + (float)(abs(10))/100.0f;
    for (int k = 0 ; k < 256 ; k++){
        SaturationLUT[k] =  CLAMP((int)((float)(k - 128) * saturation_level + 128),0,255);
    }

    int uvHeight = height/2;
    for (int y = 0; y < uvHeight; y ++) {
        for (int x = 0; x < width; x ++) {
            inputUV[y * stride + x] = SaturationLUT[inputUV[y * stride + x]];
        }
    }
    PMLOG_INFO(LOG_TAG, "<");
}

void contrastEnhancement(unsigned char* inputY, unsigned char* inputUV,
                         int width, int height, int stride, int frameSize,
                         int minY, int maxY, int enhanceLevel)
{
    PMLOG_INFO(LOG_TAG, "Contrast Enhancement working start");

    //int   ySize          = stride*height;
    //int   uvSize         = ySize / 2;
    int   low2Middle     = 127;
    //int   middle2End     = 255-low2Middle;
    int   curveLUT[256];
    int   ContrastLUT[256];
    float contrast_level = enhanceLevel;

    for(int i = 0 ; i < 256 ; i++){
        ContrastLUT[i] = i;
    }

    contrast_level = contrast_level < 1 ? 1 : contrast_level;
    memset(curveLUT, 0, sizeof(curveLUT));
    PMLOG_INFO(LOG_TAG, "contrastEnhancement start making LUT table");

    for (int k = 0 ; k <= low2Middle ; k++){
        curveLUT[k] = abs(k - (int)((float)low2Middle-(float)low2Middle*(float)(pow(double(low2Middle-k)/(double)low2Middle,(double)contrast_level))));

        int LUT_value = CLAMP(k - curveLUT[k],0,low2Middle);
        if(abs(LUT_value - k) > 100)
            ContrastLUT[k] = k;
        else
            ContrastLUT[k] = CLAMP(k - curveLUT[k],0,low2Middle);
    }

    for (int k = low2Middle + 1 ; k < 256 ; k++){
        int LUT_value = CLAMP(k + curveLUT[k - low2Middle + 1],0,255);
        if(abs(LUT_value - k) > 100)
            ContrastLUT[k] = k;
        else
            ContrastLUT[k] = CLAMP(k + curveLUT[k - low2Middle + 1],0,255);
    }

    PMLOG_INFO(LOG_TAG, "Contrast Enhancement working");

    #ifdef DUMP_ENABLED
    char filename[30];
    char filepath[100];
    snprintf(filename,sizeof(filename), "AC_Input");
    snprintf(filepath,sizeof(filepath), "/var/rootdirs/home/root/ac_dump");
    dumpFrame(inputY, inputUV, width, height, stride, frameSize, filename, filepath);
    //memset(inputY, 0, width*3);
    #endif

    for (int y = 0; y < height; y ++) {
        for (int x = 0; x < width * 2; x = x + 2) {
            #if 0
            int uv_w = x - x%2;
            int uv_h = y/2;
            int U = inputUV[uv_h*stride + uv_w];
            int V = inputUV[uv_h*stride + uv_w + 1];
            if((V < 180 && V > 130)
                && (U > 90 && U < 130))
            #endif
            inputY[y * (width * 2) + x] = ContrastLUT[inputY[y * (width * 2) + x]];
        }
    }

    #ifdef DUMP_ENABLED
    snprintf(filename,sizeof(filename), "AC_Output");
    dumpFrame(inputY, inputUV, width, height, stride, frameSize, filename, filepath);
    #endif

    PMLOG_INFO(LOG_TAG, "Contrast Enhancement working done");
}

int dumpFrame(unsigned char* inputY, unsigned char* inputUV,
              int width, int height, int stride, int frameSize,
              char* filename, char* filepath)
{
    PMLOG_INFO(LOG_TAG, ">");

    char buf[128];
    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    if(pnow == NULL){
        PMLOG_INFO(LOG_TAG,"getting time is failed so do not dump");
        return false;
    }

    snprintf(buf, 128, "%s/%d_%d_%d_%d_%s_%dx%d.yuv", filepath, pnow->tm_hour, pnow->tm_min, pnow->tm_sec,((int)tmnow.tv_usec) / 1000, filename, width, height);
    PMLOG_INFO(LOG_TAG, "path( %s )", buf);

    FILE* file_fd = fopen(buf, "wb");
    if (file_fd == 0) {
        PMLOG_INFO(LOG_TAG, "cannot open file");
        return false;
    } else {
        fwrite(((unsigned char *)inputY), 1, frameSize, file_fd);
    }
    fclose(file_fd);

    PMLOG_INFO(LOG_TAG, "<");
    return true;
}
