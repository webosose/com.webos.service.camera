/**
 * Copyright(c) 2015 by LG Electronics Inc.
 * Mobile Communication Company, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGFilmEmulator.cpp
 * @contact     camera-architect@lge.com
 *
 * Description  FilmEmulator
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
    printf("%s : E\n", __func__);

}

LGCameraSolutionAutoContrast::~LGCameraSolutionAutoContrast()
{
    printf("%s : E\n", __func__);

}

void LGCameraSolutionAutoContrast::initialize(stream_format_t streamformat, int key) {
    printf("%s : E\n", __func__);

}

std::string LGCameraSolutionAutoContrast::getSolutionStr(){
    std::string solutionStr = "AutoContrast";
    return solutionStr;
}

void LGCameraSolutionAutoContrast::processForSnapshot(void* inBuf,        stream_format_t streamformat)
{
    doAutoContrastProcessing(inBuf, streamformat);
}

void LGCameraSolutionAutoContrast::processForPreview(void* inBuf,        stream_format_t streamformat)
{
    doAutoContrastProcessing(inBuf, streamformat);
}

void LGCameraSolutionAutoContrast::doAutoContrastProcessing(void* inBuf, stream_format_t streamformat)
{
    printf("%s : E\n", __func__);

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

    Yimage  = (unsigned char*)inBuf;
    UVimage = (unsigned char*)inBuf + (stride * scanline);
    printf("%s : width(%d) height(%d) stride(%d) frameSize(%d) pixel_format(%d)\n", __func__, width, height, stride, frameSize, streamformat.pixel_format);

    contrastEnhancement(Yimage, UVimage, width, height, stride, frameSize, minY, maxY, 2.0f);

    if (retval){
        printf("%s: after AutoContrast_Execute error[%d]\n", __func__, retval);
    } else {
        printf("%s: after AutoContrast works on snapshot \n", __func__);
    }

    printf("%s: X. /n",__func__);
}

void LGCameraSolutionAutoContrast::brightnessEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel) {
    printf("%s : E\n",__func__);

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

    printf("%s : X\n",__func__);

}

void LGCameraSolutionAutoContrast::contrastEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel) {
    printf("Contrast Enhancement working start\n");

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
    printf("contrastEnhancement start making LUT table\n");

    for (int k = 0 ; k <= low2Middle ; k++){
        curveLUT[k] = abs(k - (int)((float)low2Middle-(float)low2Middle*(float)(pow(double(low2Middle-k)/(double)low2Middle,(double)contrast_level))));

        int LUT_value = CLAMP(k - curveLUT[k],0,low2Middle);
        if(abs(LUT_value - k) > 100)
            ContrastLUT[k] = k;
        else
            ContrastLUT[k] = CLAMP(k - curveLUT[k],0,low2Middle);
    }
#if 0
    for (int k = 0 ; k <= middle2End ; k++){
        curveLUT[k] = abs(k - (int)((float)middle2End-(float)middle2End*(float)(pow(double(middle2End-k)/(double)middle2End,(double)contrast_level))));
    }
#endif
    for (int k = low2Middle + 1 ; k < 256 ; k++){
        int LUT_value = CLAMP(k + curveLUT[k - low2Middle + 1],0,255);
        if(abs(LUT_value - k) > 100)
            ContrastLUT[k] = k;
        else
            ContrastLUT[k] = CLAMP(k + curveLUT[k - low2Middle + 1],0,255);
    }

    printf("Contrast Enhancement working\n");
#if 0
    unsigned char* output_image = (unsigned char*)malloc(frameSize);
    if(output_image == NULL){
        printf("Contrast Enhancement alloc failed so return\n");
        return;
    }
    memcpy(output_image, inputY, frameSize);
#endif

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

#if 0
    if(output_image != NULL){
        free(output_image);
        output_image = NULL;
    }
#endif

    printf("Contrast Enhancement working done\n");

}

int LGCameraSolutionAutoContrast::dumpFrame(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, char* filename, char* filepath)
{
    printf("%s : E",__func__);

    char buf[128];
    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    if(pnow == NULL){
        printf("%s : getting time is failed so do not dump",__func__);
        return false;
    }

    snprintf(buf, 128, "%s/%d_%d_%d_%d_%s_%dx%d.yuv", filepath, pnow->tm_hour, pnow->tm_min, pnow->tm_sec,((int)tmnow.tv_usec) / 1000, filename, width, height);
    printf("%s: path( %s )\n", __func__, buf);

    FILE* file_fd = fopen(buf, "wb");
    if (file_fd == 0)
    {
        printf("%s: cannot open file\n", __func__);
        return false;
    }
    else
    {
        fwrite(((unsigned char *)inputY), 1, frameSize, file_fd);
    }
    fclose(file_fd);

    printf("%s : X",__func__);
    return true;

}


void LGCameraSolutionAutoContrast::release() {
    printf("%s : E\n", __func__);
}

