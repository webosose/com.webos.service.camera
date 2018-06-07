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

//#include "camera_common.h"
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*fpDataCB)(int nDevNum, int nStreamType, unsigned int nDataLength,
        unsigned char *pData, unsigned int nTimestamp);
DEVICE_RETURN_CODE_T alsa_mic_open(int micNum, int samplingRate, int codec);
DEVICE_RETURN_CODE_T alsa_mic_start(int micNum);
DEVICE_RETURN_CODE_T alsa_mic_stop(int micNum);
DEVICE_RETURN_CODE_T alsa_mic_close(int micNum);
DEVICE_RETURN_CODE_T alsa_mic_get_info(int micNum, MIC_INFO_T *pInfo);
DEVICE_RETURN_CODE_T alsa_mic_get_list(int *pMicCount);
DEVICE_RETURN_CODE_T alsa_mic_set_property(int micNum, MIC_PROPERTIES_INDEX_T nProperty,
        long value);
DEVICE_RETURN_CODE_T alsa_mic_get_property(int micNum, MIC_PROPERTIES_INDEX_T nProperty,
        long *value);
void alsa_mic_register_callback(int cameraNum, fpDataCB func);
//void alsa_mic_RegisterCallback(int micNum,pDataCb func);

//Local functions

#ifdef __cplusplus
}
#endif
