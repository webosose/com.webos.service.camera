// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <alsa/control.h>
#include "hal.h"
#include "hal_cam_internal.h"
#include "hal_alsa.h"

typedef struct _MIC_DEVICE
{
    int nVirtualNumber;
    int nDevNumber;
    int nUsbPortNum;

    char strDeviceName[CONST_MAX_STRING_LENGTH];
    int nSupportChannel;
    int nSupportSamplingRate;
    int nSupportCodec;
    int nType;      // microphone
    int bCheckFlag;   // unplug

    // default properties
    int nDefaultMaxVolume;
    int nDefaultMinVolume;
    int nDefaultVolume;
    int nDefaultMute;

    // handler
    int nHandlerNumber;
    char strCaptureName[CONST_MAX_STRING_LENGTH];
    char strMixerName[CONST_MAX_STRING_LENGTH];
    snd_pcm_t *pCaptureHandle;
    snd_mixer_t *pMixerHandle;
    snd_mixer_elem_t *pElementHandle;

    pthread_t nCaptureThread;
    // properties
    int nChannel;
    CAMERA_MICROPHONE_SAMPLINGRATE_T nSamplingRate;
    CAMERA_MICROPHONE_FORMAT_T nCodec;
    unsigned int nSamplingRateNumber;
    long nVolume;
    long nMaxVolume;
    long nMinVolume;

    // control variables
    int bSetCapturing;
    int bSetMixer;
    int bCapturing;
    int bStarted;
    int bMute;

    sem_t hThread;

    // Callback
    fpDataCB pDataCB;
} MIC_DEVICE_T;

#define CONST_MAX_STRING_LENGTH 256

static MIC_DEVICE_T gMicList[10];

void alsa_mic_register_callback(int cameraNum, fpDataCB func)
{

    int camCnt = cameraNum;

    gMicList[camCnt].pDataCB = func;
}

DEVICE_RETURN_CODE_T mixer_open(MIC_DEVICE_T *oHandler)
{
    int err;

    if (oHandler == NULL)
        return DEVICE_ERROR_WRONG_PARAM;

    if ((err = snd_mixer_open(&oHandler->pMixerHandle, 0)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] not found: snd_mixer_open err= (%s)\n", __FUNCTION__,
                __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    if ((err = snd_mixer_attach(oHandler->pMixerHandle, oHandler->strMixerName)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] not found: snd_mixer_attach err= (%s)\n", __FUNCTION__,
                __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }

    if ((err = snd_mixer_selem_register(oHandler->pMixerHandle, NULL, NULL)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] not found: snd_mixer_selem_register err= (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }

    if ((err = snd_mixer_load(oHandler->pMixerHandle)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] not found: snd_mixer_load err= (%s)\n", __FUNCTION__,
                __LINE__, snd_strerror(err));
        snd_mixer_free(oHandler->pMixerHandle);
        return DEVICE_ERROR_UNKNOWN;
    }

    oHandler->pElementHandle = snd_mixer_first_elem(oHandler->pMixerHandle);
    oHandler->bSetMixer = 1;

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T mic_init(MIC_DEVICE_T *oHandler)
{
    char *deviceName;
    int next = -1;

    if (snd_card_next(&next) != 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "sn_card_next failed\n");
    }
    PMLOG_INFO(CONST_MODULE_DC, "next value:%d\n", next);
    if (snd_card_get_name(next + 1, &deviceName) == 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "Found ALSA : %d. %s", next + 1, deviceName);
        strncpy(oHandler->strDeviceName, deviceName, CONST_MAX_STRING_LENGTH);
        snprintf(oHandler->strCaptureName, CONST_MAX_STRING_LENGTH, "hw:%d,0", next + 1);
        snprintf(oHandler->strMixerName, CONST_MAX_STRING_LENGTH, "hw:%d", next + 1);
    }
    return DEVICE_OK;
}

static void *_mic_capture_thread(void *pHandler)
{
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] CaptureThread Started\n", __FUNCTION__, __LINE__);

    MIC_DEVICE_T *oHandler = (MIC_DEVICE_T *) pHandler;
    oHandler->nSamplingRateNumber = 16000;
    oHandler->nChannel = 2;

    int nLength;
    snd_pcm_state_t nState;
    snd_timestamp_t time;

    unsigned char *buffer;
    int nDataSize;
    if (oHandler != NULL)
    {
        buffer = (unsigned char *) malloc(
                sizeof(char)
                        * (oHandler->nSamplingRateNumber * oHandler->nChannel * 2 * 10 / 1000 + 1));

        if(NULL == buffer)
        {
            PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Malloc failed\n", __FUNCTION__, __LINE__);
            return NULL;
        }
        nDataSize = oHandler->nSamplingRateNumber * oHandler->nChannel * 2 * 10 / 1000;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] audio microphone handler create Error!!\n",
                __FUNCTION__, __LINE__);
        return NULL;
    }

    PMLOG_INFO(CONST_MODULE_DC, "%d:%s\n", __LINE__, __FUNCTION__);
    while (oHandler->bStarted)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%d:%s\n", __LINE__, __FUNCTION__);
        while (oHandler->bCapturing)
        {
            nState = snd_pcm_state(oHandler->pCaptureHandle);

            if (nState == SND_PCM_STATE_RUNNING)
            {
                do
                {
                    nLength = snd_pcm_readi(oHandler->pCaptureHandle, buffer,
                            nDataSize / oHandler->nChannel / 2);

                } while (nLength == -EAGAIN);
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_DC, "%s:%d] state is not 'SND_PCM_STATE_RUNNING'\n",
                        __FUNCTION__, __LINE__);

                break;
            }

            if (oHandler->pDataCB == NULL)
            {
                PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Callback function is null.\n", __FUNCTION__,
                        __LINE__);
            }
            else
            {
                oHandler->pDataCB(0, ST_PCM, nDataSize, buffer,
                        time.tv_sec * 1000000 + time.tv_usec);
            }
        }
        PMLOG_INFO(CONST_MODULE_DC, "%d:%s\n", __LINE__, __FUNCTION__);
    }

    free(buffer);
    sem_post(&oHandler->hThread);

    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] _MIC_MicRunThread Ended(%ld)\n", __FUNCTION__, __LINE__,
            oHandler->nCaptureThread);
    pthread_detach(oHandler->nCaptureThread);
    return NULL;
}

DEVICE_RETURN_CODE_T mic_open(MIC_DEVICE_T *oHandler)
{
    snd_pcm_hw_params_t *pHWParams;
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;

    PMLOG_INFO(CONST_MODULE_DC, "\ncapture name:%s\n", oHandler->strCaptureName);
    if ((err = snd_pcm_open(&oHandler->pCaptureHandle, oHandler->strCaptureName,
            SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot open audio device (%s)\n", __FUNCTION__,
                __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_open success\n", __LINE__);
    if ((err = snd_pcm_hw_params_malloc(&pHWParams)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot allocate hardware parameter structure (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params_malloc success\n", __LINE__);

    if ((err = snd_pcm_hw_params_any(oHandler->pCaptureHandle, pHWParams)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot initialize hardware parameter structure (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params_any success\n", __LINE__);

    if ((err = snd_pcm_hw_params_set_access(oHandler->pCaptureHandle, pHWParams,
            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot set access type (%s)\n", __FUNCTION__, __LINE__,
                snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params_set_access success\n", __LINE__);

    if ((err = snd_pcm_hw_params_set_format(oHandler->pCaptureHandle, pHWParams,
            SND_PCM_FORMAT_S16_LE)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot set sample format (%s)\n", __FUNCTION__,
                __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params_set_format success\n", __LINE__);

    oHandler->nSamplingRateNumber = 24000;
    if ((err = snd_pcm_hw_params_set_rate_near(oHandler->pCaptureHandle, pHWParams,
            &oHandler->nSamplingRateNumber, 0)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot set sample rate (%s)\n", __FUNCTION__, __LINE__,
                snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params_set_rate_near success\n", __LINE__);

    if ((err = snd_pcm_hw_params_set_channels(oHandler->pCaptureHandle, pHWParams, 2)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot set channel count (%s)\n", __FUNCTION__,
                __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params_set_channels success\n", __LINE__);
    if ((err = snd_pcm_hw_params(oHandler->pCaptureHandle, pHWParams)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot set parameters (%s)\n", __FUNCTION__, __LINE__,
                snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%d: snd_pcm_hw_params success\n", __LINE__);

    snd_pcm_hw_params_free(pHWParams);

    oHandler->bSetCapturing = 1;
    oHandler->bStarted = 1;
    return ret;
}

DEVICE_RETURN_CODE_T alsa_mic_get_property(int micNum, MIC_PROPERTIES_INDEX_T nProperty,
        long *value)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];
    long nMinVol = 0;
    long nMaxVol = 0;

    if (oHandler == NULL || oHandler->pElementHandle == NULL || value == NULL)
        return DEVICE_ERROR_WRONG_PARAM;
    switch (nProperty)
    {
    case MIC_PROPERTIES_MUTE:
    {
        if (value)
        {
            oHandler->bMute = 1;
        }
        else
        {
            oHandler->bMute = 0;
        }
        break;
    }
    case MIC_PROPERTIES_MAXGAIN:
    {
        if ((err = snd_mixer_selem_get_capture_volume_range(oHandler->pElementHandle, &nMinVol,
                &nMaxVol)) < 0)
        {
            PMLOG_INFO(CONST_MODULE_DC,
                    "%s:%d] cannot snd_mixer_selem_set_capture_volume_range (%s)\n", __FUNCTION__,
                    __LINE__, snd_strerror(err));
            return DEVICE_ERROR_UNKNOWN;
        }
        else
        {
            oHandler->nMaxVolume = nMaxVol;
            oHandler->nMinVolume = nMinVol;
            *value = nMaxVol;
        }
        break;
    }
    case MIC_PROPERTIES_MINGAIN:
    {
        if ((err = snd_mixer_selem_get_capture_volume_range(oHandler->pElementHandle, &nMinVol,
                &nMaxVol)) < 0)
        {
            PMLOG_INFO(CONST_MODULE_DC,
                    "%s:%d] cannot snd_mixer_selem_set_capture_volume_range (%s)\n", __FUNCTION__,
                    __LINE__, snd_strerror(err));
            return DEVICE_ERROR_UNKNOWN;
        }
        else
        {
            oHandler->nMaxVolume = nMaxVol;
            oHandler->nMinVolume = nMinVol;
            *value = nMinVol;
        }
        break;
    }
    default:
    {
        PMLOG_INFO(CONST_MODULE_DC, "Invalid property value\n");
        break;
    }
    }
    return ret;
}

DEVICE_RETURN_CODE_T alsa_mic_set_property(int micNum, MIC_PROPERTIES_INDEX_T nProperty, long value)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];
    long nMinVol = 0;
    long nMaxVol = 0;

    if (oHandler == NULL || oHandler->pElementHandle == NULL)
        return DEVICE_ERROR_WRONG_PARAM;
    switch (nProperty)
    {
    case MIC_PROPERTIES_MUTE:
    {
        if (value)
        {
            oHandler->bMute = 1;
        }
        else
        {
            oHandler->bMute = 0;
        }
        break;
    }
    case MIC_PROPERTIES_MAXGAIN:
    {
        nMinVol = oHandler->nMinVolume;
        nMaxVol = value;
        if ((err = snd_mixer_selem_set_capture_volume_range(oHandler->pElementHandle, nMinVol,
                nMaxVol)) < 0)
        {
            PMLOG_INFO(CONST_MODULE_DC,
                    "%s:%d] cannot snd_mixer_selem_set_capture_volume_range (%s)\n", __FUNCTION__,
                    __LINE__, snd_strerror(err));
            return DEVICE_ERROR_UNKNOWN;
        }
        else
        {
            oHandler->nMaxVolume = nMaxVol;
        }
        break;
    }
    case MIC_PROPERTIES_MINGAIN:
    {
        nMinVol = value;
        nMaxVol = oHandler->nMaxVolume;
        if ((err = snd_mixer_selem_set_capture_volume_range(oHandler->pElementHandle, nMinVol,
                nMaxVol)) < 0)
        {
            PMLOG_INFO(CONST_MODULE_DC,
                    "%s:%d] cannot snd_mixer_selem_set_capture_volume_range (%s)\n", __FUNCTION__,
                    __LINE__, snd_strerror(err));
            return DEVICE_ERROR_UNKNOWN;
        }
        else
        {
            oHandler->nMinVolume = nMinVol;
        }
        break;
    }
    case MIC_PROPERTIES_GAIN:
    {
        if ((err = snd_mixer_selem_set_capture_volume_all(oHandler->pElementHandle, value)) < 0)
        {
            PMLOG_INFO(CONST_MODULE_DC,
                    "%s:%d] cannot snd_mixer_selem_set_capture_volume_range (%s)\n", __FUNCTION__,
                    __LINE__, snd_strerror(err));
            return DEVICE_ERROR_UNKNOWN;
        }
        else
        {
            oHandler->nVolume = value;
        }
        break;
    }
    default:
    {
        PMLOG_INFO(CONST_MODULE_DC, "Invalid property value\n");
        break;
    }
    }
    return ret;
}

DEVICE_RETURN_CODE_T alsa_mic_get_list(int *pMicCount)
{
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Started !!!\n", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;
    char *strDevName;
    char strCaptureName[CONST_MAX_STRING_LENGTH];
    snd_pcm_t *pCaptureHandle;
    int i;
    int nDevNum;
    int next;

    next = -1;
    nDevNum = 0;
    while (1)
    {
        AGAIN: if (snd_card_next(&next) != 0)
            break;

        if (next < 0)
            break;

        if (snd_card_get_name(next, &strDevName) == 0)
        {
            PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Found! : ALSA - %d. %s\n", __FUNCTION__, __LINE__,
                    (next + 1), strDevName);

            snprintf(strCaptureName, CONST_MAX_STRING_LENGTH, "hw:%d,0", next);
            if ((err = snd_pcm_open(&pCaptureHandle, strCaptureName, SND_PCM_STREAM_CAPTURE, 0))
                    < 0)
            {
                PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot open audio device (%s)\n", __FUNCTION__,
                        __LINE__, snd_strerror(err));
                goto AGAIN;
            }

            if ((err = snd_pcm_close(pCaptureHandle)) < 0)
            {
                PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot prepare audio interface for use (%s)\n",
                        __FUNCTION__, __LINE__, snd_strerror(err));
            }
            strncpy(gMicList[nDevNum].strDeviceName, strDevName, CONST_MAX_STRING_LENGTH);
            // alsa value
            gMicList[nDevNum].nHandlerNumber = next;
            snprintf(gMicList[nDevNum].strCaptureName, CONST_MAX_STRING_LENGTH, "hw:%d,0", next);
            snprintf(gMicList[nDevNum].strMixerName, CONST_MAX_STRING_LENGTH, "hw:%d", next);
            nDevNum++;
        }
    }

    *pMicCount = nDevNum;

    PMLOG_INFO(CONST_MODULE_DC, "==== ALSA MIC List (%d) ======", *pMicCount);

    for (i = 0; i < *pMicCount; i++)
        PMLOG_INFO(CONST_MODULE_DC, "%d. %s - %s - HW:%d", (i + 1), gMicList[i].strDeviceName,
                gMicList[i].strCaptureName, gMicList[i].nHandlerNumber);
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Ended!\n", __FUNCTION__, __LINE__);
    return ret;

}
DEVICE_RETURN_CODE_T alsa_mic_get_info(int micNum, MIC_INFO_T *pInfo)
{
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Started !!!\n", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];

    strncpy(pInfo->strName, oHandler->strDeviceName, CONST_MAX_STRING_LENGTH);
    pInfo->nSamplingRate = MICROPHONE_SAMPLINGRATE_WIDE_BAND;
    pInfo->nFormat = MICROPHONE_FORMAT_PCM;

    return ret;
}

DEVICE_RETURN_CODE_T alsa_mic_close(int micNum)
{
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Started !!!\n", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];

    if (oHandler == NULL)
        return DEVICE_ERROR_WRONG_PARAM;
    if ((err = snd_pcm_close(oHandler->pCaptureHandle)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot prepare audio interface for use (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    oHandler->bStarted = false;
    return ret;
}
DEVICE_RETURN_CODE_T alsa_mic_stop(int micNum)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];

    oHandler->bCapturing = 0;
    if (oHandler == NULL)
        return DEVICE_ERROR_WRONG_PARAM;
    if ((err = snd_pcm_drop(oHandler->pCaptureHandle)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot prepare audio interface for use (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }

    return ret;
}
DEVICE_RETURN_CODE_T alsa_mic_start(int micNum)
{
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Started !!!\n", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int err;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];

    if (oHandler == NULL)
        return DEVICE_ERROR_WRONG_PARAM;
    if ((err = snd_pcm_prepare(oHandler->pCaptureHandle)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] cannot prepare audio interface for use (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }

    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Started !!!\n", __FUNCTION__, __LINE__);
    if ((err = snd_pcm_start(oHandler->pCaptureHandle)) < 0)
    {
        PMLOG_INFO(CONST_MODULE_DC, "%s:%d] can not start recording: snd_pcm_start err= (%s)\n",
                __FUNCTION__, __LINE__, snd_strerror(err));
        return DEVICE_ERROR_UNKNOWN;
    }
    PMLOG_INFO(CONST_MODULE_DC, "%s:%d] Started !!!\n", __FUNCTION__, __LINE__);

    oHandler->bCapturing = 1;
    return ret;

}
DEVICE_RETURN_CODE_T alsa_mic_open(int micNum, int samplingRate, int codec)
{

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    MIC_DEVICE_T *oHandler = &gMicList[micNum];

    ret = mic_init(&gMicList[micNum]);
    if (ret != DEVICE_OK)
    {
        PMLOG_INFO(CONST_MODULE_DC, "mic_open failed\n");
        return ret;
    }
    ret = mic_open(&gMicList[micNum]);
    if (ret != DEVICE_OK)
    {
        PMLOG_INFO(CONST_MODULE_DC, "mic_open failed\n");
        return ret;
    }
    ret = mixer_open(&gMicList[micNum]);
    if (ret != DEVICE_OK)
    {
        PMLOG_INFO(CONST_MODULE_DC, "mic_mixer_open failed\n");
        return ret;
    }
    if (-1
            == pthread_create(&oHandler->nCaptureThread, 0,
                    (void*(*)(void*))_mic_capture_thread, oHandler))
                    {
                        PMLOG_INFO(CONST_MODULE_DC,"%s:%d] Failed to create thread <_mic_capture_thread>\n", __FUNCTION__, __LINE__);
                        return DEVICE_ERROR_UNKNOWN;
                    }

                }

