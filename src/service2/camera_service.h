#ifndef CAMERA_SERVICE_H_
#define CAMERA_SERVICE_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "luna-service2/lunaservice.hpp"
#include <glib.h>

class CameraService : public LS::Handle
{
  private:
    using mainloop = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
    mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

  public:
    CameraService();

    CameraService(CameraService const &) = delete;
    CameraService(CameraService &&) = delete;
    CameraService &operator=(CameraService const &) = delete;
    CameraService &operator=(CameraService &&) = delete;

    bool open(LSMessage &);
    bool close(LSMessage &);
    bool getInfo(LSMessage &);
    bool getCameraList(LSMessage &);
    bool getProperties(LSMessage &);
    bool setProperties(LSMessage &);
    bool setFormat(LSMessage &);
    bool startPreview(LSMessage &);
    bool stopPreview(LSMessage &);
    bool startCapture(LSMessage &);
    bool stopCapture(LSMessage &);
    bool getList(LSMessage &);
};

#endif /*CAMERA_SERVICE_H_*/
