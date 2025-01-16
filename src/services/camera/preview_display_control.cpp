#define LOG_TAG "PreviewDisplayControl"
#include "preview_display_control.h"
#include "camera_constants.h"
#include "generate_unique_id.h"
#include "process.h"
#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>
#include <sstream>

#define LOAD_PAYLOAD_HEAD "{\"appId\":\"com.webos.app.mediaevents-test\", \"windowId\":\""

const char *display_client_service_name = "com.webos.service.camera2.display-";
const std::string window_id_str         = "_Window_Id_";

static pbnjson::JValue convertStringToJson(const char *rawData)
{
    pbnjson::JInput input(rawData);
    pbnjson::JSchema schema = pbnjson::JSchema::AllSchema();
    pbnjson::JDomParser parser;
    if (!parser.parse(input, schema))
    {
        return pbnjson::JValue();
    }
    return parser.getDom();
}

PreviewDisplayControl::PreviewDisplayControl(const std::string &wid)
    : sh_(nullptr), loop_(nullptr), reply_from_server_(""), done_(0)
{
    PLOGI("");
    acquireLSConnection(wid);
}

PreviewDisplayControl::~PreviewDisplayControl()
{
    releaseLSConnection();
    PLOGI("");
}

bool PreviewDisplayControl::isValidWindowId(std::string windowId)
{
    std::stringstream strStream;
    int num      = 0;
    int digit    = -1;
    size_t end   = std::string::npos;
    size_t begin = std::string::npos;

    begin = windowId.find(window_id_str);
    if (begin != 0)
    {
        PLOGI("Invalid windowId value");
        return false;
    }

    begin = window_id_str.length();
    end   = windowId.length();

    for (size_t index = begin; index < end; index++)
    {
        if (isdigit(static_cast<unsigned char>(windowId[index])))
        {
            strStream << windowId[index];
            strStream >> digit;
            if (!strStream)
            {
                PLOGI("Error: conversion from string to number failed");
                return false;
            }
            strStream.clear();
            if (num > INT_MAX / 10)
            {
                PLOGI("Potential overflow detected");
                return false;
            }
            num = digit + num * 10;
        }
    }
    if (num > 0)
    {
        return true;
    }
    return false;
}

bool PreviewDisplayControl::acquireLSConnection(const std::string &wid)
{
    if (!sh_)
    {
        if (!isValidWindowId(wid))
        {
            return false;
        }

        LSError lserror;
        LSErrorInit(&lserror);

        // _Window_Id_XX
        name_ = display_client_service_name + wid.substr(11);
        PLOGI("%s", name_.c_str());
        if (!LSRegister(name_.c_str(), &sh_, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            return false;
        }

        loop_ = g_main_loop_new(NULL, FALSE);

        if (!LSGmainAttach(sh_, loop_, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            g_main_loop_unref(loop_);
            LSUnregister(sh_, &lserror);
            LSErrorFree(&lserror);
            loop_ = nullptr;
            sh_   = nullptr;
            return false;
        }
    }
    return true;
}

bool PreviewDisplayControl::releaseLSConnection()
{
    if (loop_)
    {
        g_main_loop_quit(loop_);
        g_main_loop_unref(loop_);
        loop_ = nullptr;
    }
    if (sh_)
    {
        LSError lserror;
        LSErrorInit(&lserror);
        PLOGI("%s", name_.c_str());
        if (!LSUnregister(sh_, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            return false;
        }
        LSErrorFree(&lserror);
        sh_ = nullptr;
    }
    return true;
}

bool PreviewDisplayControl::call(std::string uri, std::string payload,
                                 bool (*cb)(LSHandle *, LSMessage *, void *))
{
    done_ = 0;

    LSError lserror;
    LSErrorInit(&lserror);

    GMainContext *context = g_main_loop_get_context(loop_);

    if (!LSCall(sh_, uri.c_str(), payload.c_str(), cb, this, NULL, &lserror))
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }

    LSErrorFree(&lserror);

    while (!done_)
    {
        g_main_context_iteration(context, false);
        usleep(100);
    }
    return true;
}

bool PreviewDisplayControl::cbHandleResponseMsg(LSHandle *sh, LSMessage *msg, void *ctx)
{
    PreviewDisplayControl *caller = static_cast<PreviewDisplayControl *>(ctx);
    const char *str               = LSMessageGetPayload(msg);
    PLOGI("reply: %s", str);

    // The callback may be called multiple times
    if (caller->done_ == 0)
    {
        caller->reply_from_server_ = (str) ? str : "";
        caller->done_              = 1;
    }

    return true;
}

bool PreviewDisplayControl::start(std::string camera_id, std::string windowId,
                                  CAMERA_FORMAT cameraFormat, std::string memType, int key,
                                  int handle_, bool primary)
{
    if (!isValidWindowId(windowId))
    {
        PLOGE("Invalid windowId value");
        return false;
    }

    std::string mem_type = "shmem";
    if (memType == kMemtypePosixshm)
    {
        mem_type = "posixshm";
    }

    std::string informat = getFormatStringFromCode(cameraFormat.eFormat);
    std::string outformat;
    if (informat == "YUV")
    {
        outformat = "YUY2";
    }
    else
    {
        outformat = std::move(informat);
    }

    // Create camera pipeline
    std::string guid = GenerateUniqueID()();
    std::string uid  = "com.webos.pipeline.camera." + guid;
    std::string cmd  = "/usr/sbin/g-camera-pipeline -s" + uid + " -u";
    pipeline_process = std::make_unique<Process>(cmd);

    std::string payload =
        LOAD_PAYLOAD_HEAD + windowId +
        "\", \"videoDisplayMode\":\"Textured\",\"width\":" + std::to_string(cameraFormat.nWidth) +
        ",\"height\":" + std::to_string(cameraFormat.nHeight) + ",\"format\":\"" + outformat +
        "\",\"frameRate\":" + std::to_string(cameraFormat.nFps) + ",\"memType\":\"" + mem_type +
        "\",\"memSrc\":\"" + std::to_string(key) + "\",";

    if (memType == kMemtypePosixshm)
    {
        payload += "\"handle\":" + std::to_string(handle_) + ",";
    }

    payload += "\"primary\":" + std::string(primary ? "true" : "false") + ",";
    payload += "\"cameraId\":\"" + camera_id + "\"}";

    PLOGI("payload : %s", payload.c_str());

    // send message for load
    pipeline_uri    = "luna://" + uid + "/";
    std::string uri = pipeline_uri + __func__;

    if (!call(uri.c_str(), std::move(payload), cbHandleResponseMsg))
    {
        PLOGE("fail to call load()");
        return false;
    }

    pbnjson::JValue parsed = convertStringToJson(reply_from_server_.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        PLOGE("load() FAILED");
        return false;
    }

    pid = parsed["pid"].asNumber<int32_t>();
    PLOGI("pid = %d", pid);
    handle = handle_;

    return true;
}

bool PreviewDisplayControl::stop()
{
    PLOGI("unload() starts.");

    // send message
    std::string uri = pipeline_uri + __func__;

    std::string payload = "{}";
    if (!call(uri.c_str(), std::move(payload), cbHandleResponseMsg))
    {
        PLOGE("fail to call unload()");
        return false;
    }

    pbnjson::JValue parsed = convertStringToJson(reply_from_server_.c_str());
    bool result            = parsed["returnValue"].asBool();
    PLOGI("returnValue : %d ", result);

    return result;
}
