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

    void acquireLSConnection(const std::string &wid);
    void releaseLSConnection();
    bool call(std::string uri, std::string payload, bool (*cb)(LSHandle*, LSMessage*, void*));

    bool isValidWindowId(std::string windowId);
    int getPid(std::string mediaId);

    static bool cbHandleResponseMsg(LSHandle*, LSMessage*, void*);

public:
    PreviewDisplayControl(const std::string &wid);
    ~PreviewDisplayControl();
    std::string load(std::string cameraId, std::string windowId,
                     CAMERA_FORMAT cameraFormat, std::string memType,
                     int key, int handle);
    bool play(std::string mediaId);
    bool unload(std::string mediaId);
    bool getControlStatus();
};

#endif /* PREVIEW_DISPLAY_CONTROL_H_ */