#ifndef PREVIEW_DISPLAY_CONTROL_H_
#define PREVIEW_DISPLAY_CONTROL_H_

#include "camera_types.h"
#include <luna-service2/lunaservice.h>
#include <glib.h>
#include <string>

class PreviewDisplayControl
{
private:
    std::string name_;
    LSHandle *sh_;
    GMainLoop *loop_;
    std::string reply_from_server_;
    int done_;
    bool bResult_;

    void acquireLSConnection();
    void releaseLSConnection();
    bool call(std::string uri, std::string payload, bool (*cb)(LSHandle*, LSMessage*, void*));

    static bool cbHandleResponseMsg(LSHandle*, LSMessage*, void*);

public:
    PreviewDisplayControl();
    ~PreviewDisplayControl();
    std::string load(std::string cameraId, std::string windowId,
                     CAMERA_FORMAT cameraFormat, std::string memType,
                     int key, int handle);
    void play(std::string mediaId);
    void unload(std::string mediaId);
    bool getControlStatus();
};

#endif /* PREVIEW_DISPLAY_CONTROL_H_ */