#include "preview_display_control.h"
#include "constants.h"
#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>
#include <sstream>

#define LOAD_PAYLOAD_HEAD "{\"uri\":\"camera://com.webos.service.camera2/7010\",\"payload\":\
{\"option\":{\"appId\":\"com.webos.app.mediaevents-test\", \"windowId\":\""

const char *display_client_service_name = "com.webos.service.camera2.display-";
const std::string window_id_str = "_Window_Id_";

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

PreviewDisplayControl::PreviewDisplayControl(const std::string &wid) :
    sh_(nullptr),
    loop_(nullptr),
    reply_from_server_(""),
    done_(0),
    bResult_(true)
{
    PMLOG_INFO(CONST_MODULE_DPY, "");
    acquireLSConnection(wid);
}

PreviewDisplayControl::~PreviewDisplayControl()
{
    releaseLSConnection();
    PMLOG_INFO(CONST_MODULE_DPY, "");
}

bool PreviewDisplayControl::isValidWindowId(std::string windowId)
{
    std::stringstream strStream;
    int num = 0;
    int digit = -1;;
    int end = -1;
    int begin = -1;

    begin = windowId.find(window_id_str);
    if (begin != 0)
    {
        PMLOG_INFO(CONST_MODULE_DPY, "Invalid windowId value");
        bResult_ = false;
        return false;
    }

    begin = window_id_str.length();
    end = windowId.length();

    for (int index = begin; index < end; index++)
    {
        if (isdigit(windowId[index]))
        {
            strStream << windowId[index];
            strStream >> digit;
            strStream.clear();
            num = digit + num * 10;
        }
    }
    if (num > 0)
    {
        return true;
    }
    return false;
}

void PreviewDisplayControl::acquireLSConnection(const std::string &wid)
{
    if (!sh_)
    {
        if (!isValidWindowId(wid))
        {
            bResult_ = false;
            return;
        }

        LSError lserror;
        LSErrorInit(&lserror);

        // _Window_Id_XX
        name_ = display_client_service_name + wid.substr(11);
        PMLOG_INFO(CONST_MODULE_DPY, "%s", name_.c_str());
        if (!LSRegister(name_.c_str(), &sh_, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            bResult_ = false;
            return;
        }

        loop_ = g_main_loop_new(NULL, FALSE);

        if (!LSGmainAttach(sh_, loop_, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            g_main_loop_unref(loop_);
            LSUnregister(sh_, &lserror);
            LSErrorFree(&lserror);
            loop_ = nullptr;
            sh_ = nullptr;
            bResult_ = false;
            return;
        }
    }
    bResult_ = true;
}

void PreviewDisplayControl::releaseLSConnection()
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
        PMLOG_INFO(CONST_MODULE_DPY, "%s", name_.c_str());
        if (!LSUnregister(sh_, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            bResult_ = false;
            return;
        }
        LSErrorFree(&lserror);
        sh_ = nullptr;
    }
    bResult_ = true;
}

bool PreviewDisplayControl::call(std::string uri, std::string payload, bool (*cb)(LSHandle*, LSMessage*, void*))
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
    PreviewDisplayControl *caller = static_cast<PreviewDisplayControl*>(ctx);
    const char *str = LSMessageGetPayload(msg);
    caller->reply_from_server_ = (str) ? str : "";
    caller->done_ = 1;
    PMLOG_INFO(CONST_MODULE_DPY, "reply_from_server: %s", caller->reply_from_server_.c_str());
    return true;
}

std::string PreviewDisplayControl::load(std::string camera_id, std::string windowId,
                                        CAMERA_FORMAT cameraFormat, std::string memType,
                                        int key, int handle, bool primary)
{
    std::string media_id = "";

    if (!isValidWindowId(windowId))
    {
        PMLOG_INFO(CONST_MODULE_DPY, "Invalid windowId value");
        bResult_ = false;
        return media_id;
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

    std::string payload = LOAD_PAYLOAD_HEAD
                        + windowId + "\", \"videoDisplayMode\":\"Textured\",\"width\":"
                        + std::to_string(cameraFormat.nWidth) + ",\"height\":"
                        + std::to_string(cameraFormat.nHeight) + ",\"format\":\""
                        + outformat + "\",\"frameRate\":"
                        + std::to_string(cameraFormat.nFps) + ",\"memType\":\"" + mem_type
                        + "\",\"memSrc\":\"" + std::to_string(key) + "\",";

    if (memType == kMemtypePosixshm)
    {
        payload +=  "\"handle\":" + std::to_string(handle) + ",";
    }

    payload += "\"primary\":" + std::string(primary ? "true" : "false") + ",";
    payload += "\"cameraId\":\"" + camera_id + "\"}},\"type\":\"camera\"}";

    PMLOG_INFO(CONST_MODULE_DPY, "payload : %s", payload.c_str());

    if (!call("luna://com.webos.media/load", std::move(payload), cbHandleResponseMsg))
    {
        PMLOG_INFO(CONST_MODULE_DPY, "fail to call com.webos.media/load()");
        bResult_ = false;
        return media_id;
    }

    pbnjson::JValue parsed = convertStringToJson(reply_from_server_.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        PMLOG_INFO(CONST_MODULE_DPY, "load() FAILED");
        bResult_ = false;
        return media_id;
    }
    bResult_ = true;
    media_id = parsed["mediaId"].asString();
    PMLOG_INFO(CONST_MODULE_DPY, "mediaId = %s ", media_id.c_str());

    return media_id;
}

bool PreviewDisplayControl::play(std::string mediaId)
{
    std::string payload = "{\"mediaId\":\"" + mediaId + "\"}";
    if (!call("luna://com.webos.media/play", std::move(payload), cbHandleResponseMsg))
    {
        PMLOG_INFO(CONST_MODULE_DPY, "fail to call com.webos.media/play()");
        bResult_ = false;
        return false;
    }

    pbnjson::JValue parsed = convertStringToJson(reply_from_server_.c_str());
    bResult_ = parsed["returnValue"].asBool();
    PMLOG_INFO(CONST_MODULE_DPY, "returnValue : %d ", bResult_);

    return bResult_;
}

bool PreviewDisplayControl::unload(std::string mediaId)
{
    PMLOG_INFO(CONST_MODULE_DPY, "unload() starts.");

    getPid(mediaId);

    std::string payload = "{\"mediaId\":\"" + mediaId + "\"}";
    if (!call("luna://com.webos.media/unload", std::move(payload), cbHandleResponseMsg))
    {
        PMLOG_INFO(CONST_MODULE_DPY, "fail to call com.webos.media/unload()");
        bResult_ = false;
        return false;
    }

    pbnjson::JValue parsed = convertStringToJson(reply_from_server_.c_str());
    bResult_ =  parsed["returnValue"].asBool();
    PMLOG_INFO(CONST_MODULE_DPY, "returnValue : %d ", bResult_);

    return bResult_;
}

bool PreviewDisplayControl::getControlStatus()
{
    return bResult_;
}

int PreviewDisplayControl::getPid(std::string mediaId)
{
    PMLOG_INFO(CONST_MODULE_DPY, "mediaId: %s", mediaId.c_str());
    int pid = -1;
    std::string payload = "{\"mediaId\":\"" + mediaId + "\"}";
    if (!call("luna://com.webos.media/getActivePipelines", std::move(payload), cbHandleResponseMsg))
    {
        bResult_ = false;
        return pid;
    }

    pbnjson::JValue parsed = convertStringToJson(reply_from_server_.c_str());
    for (ssize_t i = 0; i < parsed.arraySize(); i++)
    {
        if (parsed[i]["mediaId"].asString() == mediaId)
        {
            pid = parsed[i]["pid"].asNumber<int32_t>();
            break;
        }
    }

    PMLOG_INFO(CONST_MODULE_DPY, "g-camera-pipeline PID = %d", pid);

    return pid;
}

