/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionAutoContrast.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  AutoContrast
 *
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/time.h>

#include "LGCameraSolutionAutoContrast.h"

LGCameraSolutionAutoContrast::LGCameraSolutionAutoContrast(CameraSolutionManager *mgr)
        : CameraSolution(mgr)
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n");
    solutionProperty = LG_SOLUTION_PREVIEW | LG_SOLUTION_VIDEO | LG_SOLUTION_SNAPSHOT;
}

LGCameraSolutionAutoContrast::~LGCameraSolutionAutoContrast()
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n", __func__);
    setEnableValue(false);
}

void LGCameraSolutionAutoContrast::initialize(stream_format_t streamformat) {
    PMLOG_INFO(CONST_MODULE_SM, " E\n", __func__);

}

std::string LGCameraSolutionAutoContrast::getSolutionStr(){
    std::string solutionStr = SOLUTION_AUTOCONTRAST;
    return solutionStr;
}

void LGCameraSolutionAutoContrast::processForSnapshot(buffer_t inBuf,        stream_format_t streamformat)
{
    doAutoContrastProcessing(inBuf, streamformat);
}

void LGCameraSolutionAutoContrast::processForPreview(buffer_t inBuf,        stream_format_t streamformat)
{
    //AutoContrast is only working on YUYV format currently
    if(streamformat.pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
    {
        doAutoContrastProcessing(inBuf, streamformat);
    }
}

void LGCameraSolutionAutoContrast::doAutoContrastProcessing(buffer_t inBuf, stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n");

    uint8_t *Yimage = NULL, *UVimage = NULL;
    void    *handle = NULL;
    int     retval = 0;

    int width = streamformat.stream_width;
    int height = streamformat.stream_height;
    int stride = streamformat.stream_width;
    int scanline = streamformat.stream_height;
    int frameSize = streamformat.buffer_size;
    int minY = 30;
    int maxY = 210;

    Yimage  = (unsigned char*)inBuf.start;
    UVimage = (unsigned char*)inBuf.start + (stride * scanline);
    PMLOG_INFO(CONST_MODULE_SM, "%s : width(%d) height(%d) stride(%d) frameSize(%d) pixel_format(%d)\n", __func__, width, height, stride, frameSize, streamformat.pixel_format);

    contrastEnhancement(Yimage, UVimage, width, height, stride, frameSize, minY, maxY, 2.0f);

    if (retval){
        PMLOG_INFO(CONST_MODULE_SM, "%s: after AutoContrast_Execute error[%d]\n", __func__, retval);
    } else {
        PMLOG_INFO(CONST_MODULE_SM, "%s: after AutoContrast works on snapshot \n", __func__);
    }

    PMLOG_INFO(CONST_MODULE_SM, " X. /n",__func__);
}

void LGCameraSolutionAutoContrast::brightnessEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel) {
    PMLOG_INFO(CONST_MODULE_SM, " E\n");

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

    PMLOG_INFO(CONST_MODULE_SM, " X\n");

}

void LGCameraSolutionAutoContrast::contrastEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel) {
    PMLOG_INFO(CONST_MODULE_SM, "Contrast Enhancement working start\n");

    int   ySize          = stride*height;
    int   uvSize         = ySize / 2;
    int   low2Middle     = 127;
    int   middle2End     = 255-low2Middle;
    int   curveLUT[256];
    int   ContrastLUT[256];
    float contrast_level = enhanceLevel;

    for(int i = 0 ; i < 256 ; i++){
        ContrastLUT[i] = i;
    }

    contrast_level = contrast_level < 1 ? 1 : contrast_level;
    memset(curveLUT, 0, sizeof(curveLUT));
    PMLOG_INFO(CONST_MODULE_SM, "contrastEnhancement start making LUT table\n");

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

    PMLOG_INFO(CONST_MODULE_SM, "Contrast Enhancement working\n");

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


    PMLOG_INFO(CONST_MODULE_SM, "Contrast Enhancement working done\n");

}

int LGCameraSolutionAutoContrast::dumpFrame(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, char* filename, char* filepath)
{
    PMLOG_INFO(CONST_MODULE_SM,  " E\n");

    char buf[128];
    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    if(pnow == NULL){
        PMLOG_INFO(CONST_MODULE_SM,"%s : getting time is failed so do not dump",__func__);
        return false;
    }

    snprintf(buf, 128, "%s/%d_%d_%d_%d_%s_%dx%d.yuv", filepath, pnow->tm_hour, pnow->tm_min, pnow->tm_sec,((int)tmnow.tv_usec) / 1000, filename, width, height);
    PMLOG_INFO(CONST_MODULE_SM,  "%s: path( %s )\n", __func__, buf);

    FILE* file_fd = fopen(buf, "wb");
    if (file_fd == 0)
    {
        PMLOG_INFO(CONST_MODULE_SM,  "%s: cannot open file\n", __func__);
        return false;
    }
    else
    {
        fwrite(((unsigned char *)inputY), 1, frameSize, file_fd);
    }
    fclose(file_fd);

    PMLOG_INFO(CONST_MODULE_SM, " X",__func__);
    return true;

}


void LGCameraSolutionAutoContrast::release() {
    PMLOG_INFO(CONST_MODULE_SM,  "%s : E\n", __func__);
}

