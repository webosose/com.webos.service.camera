#ifndef CAMERA_SERVICE_LOG_MESSAGES_H_
#define CAMERA_SERVICE_LOG_MESSAGES_H_

#include "PmLogLib.h"

#define PMLOG_ERROR(module, args...) PmLogMsg(getCameraLunaPmLogContext(), Error, module, 0, ##args)
#define PMLOG_INFO(module, FORMAT__, ...)                                                          \
    PmLogInfo(getCameraLunaPmLogContext(), module, 0, "%s():%d " FORMAT__, __FUNCTION__, __LINE__, \
              ##__VA_ARGS__)
#define PMLOG_DEBUG(FORMAT__, ...)                                                                 \
    PmLogDebug(getCameraLunaPmLogContext(), "[%s:%d]" FORMAT__, __PRETTY_FUNCTION__, __LINE__,     \
               ##__VA_ARGS__)

static inline PmLogContext getCameraLunaPmLogContext()
{
    static PmLogContext usLogContext = 0;
    if (0 == usLogContext)
    {
        PmLogGetContext("camera", &usLogContext);
    }
    return usLogContext;
}

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG "camera"
#endif

/*
 * Simplified macro to send a info log message using the current LOG_TAG.
 */
#ifndef PLOGI
#define PLOGI(FORMAT__, ...)                                                                       \
    PmLogInfo(getCameraLunaPmLogContext(), LOG_TAG, 0, "%s():%d " FORMAT__, __FUNCTION__,          \
              __LINE__, ##__VA_ARGS__)
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef PLOGW
#define PLOGW(FORMAT__, ...)                                                                       \
    PmLogWarning(getCameraLunaPmLogContext(), LOG_TAG, 0, "%s():%d " FORMAT__, __FUNCTION__,       \
                 __LINE__, ##__VA_ARGS__)
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef PLOGE
#define PLOGE(FORMAT__, ...)                                                                       \
    PmLogError(getCameraLunaPmLogContext(), LOG_TAG, 0, "%s():%d " FORMAT__, __FUNCTION__,         \
               __LINE__, ##__VA_ARGS__)
#endif

/*
 * Macro to send a debug log message.
 */
#ifndef PLOGD
#define PLOGD(FORMAT__, ...)                                                                       \
    PmLogDebug(getCameraLunaPmLogContext(), "[%s:%d]" FORMAT__, __PRETTY_FUNCTION__, __LINE__,     \
               ##__VA_ARGS__)
#endif

#endif /* CAMERA_SERVICE_LOG_MESSAGES_H_ */
