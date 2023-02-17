#ifndef CAMERA_SERVICE_LOG_MESSAGES_H_
#define CAMERA_SERVICE_LOG_MeSSAGES_H_

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

#endif /* CAMERA_SERVICE_LOG_MESSAGES_H_ */
