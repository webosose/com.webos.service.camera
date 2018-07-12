#ifndef SRC_SERVICE_SERVICE_MAIN_H_
#define SRC_SERVICE_SERVICE_MAIN_H_

#ifdef  __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "PmLogLib.h"
#include "json.h"
#include "luna-service2/lunaservice.h"

#include "libudev.h"
#include "constants.h"
#include "camera_types.h"

class CameraService
{
private:
    static CameraService *servicehandlerinstance;
    CameraService()
    {
    }

public:
    static CameraService *getInstance()
    {
        if (servicehandlerinstance == 0)
        {
            servicehandlerinstance = new CameraService();
        }

        return servicehandlerinstance;
    }
    ;

public:
    static bool open(LSHandle *sh, LSMessage *message, void *ctx);
    static bool close(LSHandle *sh, LSMessage *message, void *ctx);
    static bool getInfo(LSHandle *sh, LSMessage *message, void *ctx);
    static bool getCameralist(LSHandle *sh, LSMessage *message, void *ctx);
    static bool getProperties(LSHandle *sh, LSMessage *message, void *ctx);
    static bool setProperties(LSHandle *sh, LSMessage *message, void *ctx);
    static bool setFormat(LSHandle *sh, LSMessage *message, void *ctx);
    static bool startPreview(LSHandle *sh, LSMessage *message, void *ctx);
    static bool stopPreview(LSHandle *sh, LSMessage *message, void *ctx);
    static bool startCapture(LSHandle *sh, LSMessage *message, void *ctx);
    static bool stopCapture(LSHandle *sh, LSMessage *message, void *ctx);
};

LSHandle* camera_ls2_getHandle(void);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /*SRC_SERVICE_SERVICE_MAIN_H_*/

