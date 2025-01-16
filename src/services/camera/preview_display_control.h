#ifndef PREVIEW_DISPLAY_CONTROL_H_
#define PREVIEW_DISPLAY_CONTROL_H_

#include "camera_types.h"
#include <glib.h>
#include <luna-service2/lunaservice.h>
#include <memory>
#include <string>

class Process;
class PreviewDisplayControl
{
private:
    std::string name_;
    LSHandle *sh_;
    GMainLoop *loop_;
    std::string reply_from_server_;
    int done_;
    std::unique_ptr<Process> pipeline_process{nullptr};
    std::string pipeline_uri;
    int pid    = -1;
    int handle = -1;

    bool acquireLSConnection(const std::string &wid);
    bool releaseLSConnection();
    bool call(std::string uri, std::string payload, bool (*cb)(LSHandle *, LSMessage *, void *));

    bool isValidWindowId(std::string windowId);

    static bool cbHandleResponseMsg(LSHandle *, LSMessage *, void *);

public:
    PreviewDisplayControl(const std::string &wid);
    ~PreviewDisplayControl();
    bool start(std::string camera_id, std::string windowId, CAMERA_FORMAT cameraFormat, int handle,
               bool primary);
    bool stop();
    int getPid() const { return pid; }
    int getHandle() const { return handle; }
};

#endif /* PREVIEW_DISPLAY_CONTROL_H_ */