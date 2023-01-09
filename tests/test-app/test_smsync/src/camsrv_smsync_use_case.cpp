#include "camsrv_smsync_use_case.h"
#include "cmdpipeio.h"
#include "jsonparse.h"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>

std::string cmd_getcameralist =
    "luna-send -n 1 -f luna://com.webos.service.camera2/getCameraList '{}'";
std::string cmd_getinfo = "luna-send -n 1 -f luna://com.webos.service.camera2/getInfo '{\"id\": \"";
std::string cmd_open    = "luna-send -n 1 -f luna://com.webos.service.camera2/open '{\"id\": \"";
std::string cmd_setformat    = "luna-send -n 1 -f "
                               "luna://com.webos.service.camera2/setFormat "
                               "'{\"handle\":";
std::string cmd_startpreview = "luna-send -n 1 -f "
                               "luna://com.webos.service.camera2/startPreview "
                               "'{\"handle\":";
std::string cmd_stoppreview  = "luna-send -n 1 -f "
                               "luna://com.webos.service.camera2/stopPreview "
                               "'{\"handle\":";
std::string cmd_close = "luna-send -n 1 -f luna://com.webos.service.camera2/close '{\"handle\":";

std::string getStringFromNumber(int number)
{
    std::ostringstream str1;
    str1 << number;
    return str1.str();
}

ret_val_t open_start_stop_close(int dummy_pid, int dummy_sig)
{
    bool bRetVal = false;
    int handle   = -1;
    int key      = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // open
    command = cmd_open + "camera1\"}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed  = convertStringToJson(output.c_str());
    bRetVal = parsed["returnValue"].asBool();
    if (bRetVal == false)
    {
        retval = {1, {0, 0}};
        return retval;
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startpreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed  = convertStringToJson(output.c_str());
    bRetVal = parsed["returnValue"].asBool();
    if (bRetVal == false)
    {
        retval = {2, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // Do something
    for (int i = 0; i < 10; i++)
    {
        std::cout << i << ": app is doing something ... " << std::endl;
        sleep(1);
    }

    // stoppreviewa
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed  = convertStringToJson(output.c_str());
    bRetVal = parsed["returnValue"].asBool();
    if (bRetVal == false)
    {
        retval = {3, {0, 0}};
        return retval;
    }

    // close
    command = cmd_close + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed  = convertStringToJson(output.c_str());
    bRetVal = parsed["returnValue"].asBool();
    if (bRetVal == false)
    {
        retval = {4, 0};
        return retval;
    }

    return retval;
}

ret_val_t open_pid_start_stop_close_pid(int ctx_pid, int dum_sig)
{
    pid_t pid = (pid_t)-1;
    sigset_t set;
    int signum;
    int handle = -1;
    int key    = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = ctx_pid;
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {5, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {6, {0, 0}};
        return retval;
    }
    if (SIGUSR1 != parsed["sig"].asNumber<int>())
    {
        retval = {7, {0, 0}};
        return retval;
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startPreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {8, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 5;
    timeout.tv_nsec = 0;

    // test for 10 frames
    for (int i = 0; i < 10; i++)
    {
        std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
        signum = sigtimedwait(&set, NULL, &timeout);
        if (signum != -1)
        {
            std::cout << "received" << std::endl;
            // Read Shared Memeory here and do somothing
        }
        else
        {
            std::cout << "timeout (10sec)" << std::endl;
            std::cout << "server stopped notifying write event." << std::endl;
            std::cout << "app termination sequence starts ..." << std::endl;
            break;
        }
    }

    // stopPreview
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {9, {0, 0}};
        return retval;
    }

    // close
    command =
        cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {10, {handle, pid}};
        return retval;
    }

    return retval;
}

ret_val_t open_pid_sig_start_stop_close_pid(int ctx_pid, int ctx_sig)
{
    pid_t pid = (pid_t)-1;
    int sig   = ctx_sig;
    sigset_t set;
    int signum;
    int handle = -1;
    int key    = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = ctx_pid;
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) +
              ", \"sig\": " + getStringFromNumber(sig) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {11, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {12, {0, 0}};
        return retval;
    }
    if (parsed.hasKey("sig"))
    {
        if (sig != parsed["sig"].asNumber<int>())
        {
            retval = {13, {0, 0}};
            return retval;
        }
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startPreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {14, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 5;
    timeout.tv_nsec = 0;

    // test for 10 frames
    for (int i = 0; i < 10; i++)
    {
        std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
        signum = sigtimedwait(&set, NULL, &timeout);
        if (signum != -1)
        {
            std::cout << "received" << std::endl;
            // Read Shared Memeory here and do somothing
        }
        else
        {
            std::cout << "timeout (10sec)" << std::endl;
            std::cout << "server stopped notifying write event." << std::endl;
            std::cout << "app termination sequence starts ..." << std::endl;
            break;
        }
    }

    // stopPreview
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {15, {0, 0}};
        return retval;
    }

    // close
    command =
        cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {16, {0, 0}};
        return retval;
    }

    return retval;
}

ret_val_t open_pid_start_stop_close(int ctx_pid, int dummy_sig)
{
    pid_t pid = (pid_t)-1;
    sigset_t set;
    int signum;
    int handle = -1;
    int key    = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = ctx_pid;
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {17, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {18, {0, 0}};
        return retval;
    }
    if (SIGUSR1 != parsed["sig"].asNumber<int>())
    {
        retval = {19, {0, 0}};
        return retval;
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startPreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {20, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 5;
    timeout.tv_nsec = 0;

    // test for 10 frames
    for (int i = 0; i < 10; i++)
    {
        std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
        signum = sigtimedwait(&set, NULL, &timeout);
        if (signum != -1)
        {
            std::cout << "received" << std::endl;
            // Read Shared Memeory here and do somothing
        }
        else
        {
            std::cout << "timeout (10sec)" << std::endl;
            std::cout << "server stopped notifying write event." << std::endl;
            std::cout << "app termination sequence starts ..." << std::endl;
            break;
        }
    }

    // stopPreview
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {21, {0, 0}};
        return retval;
    }

    // close
    command = cmd_close + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {22, handle, pid};
        return retval;
    }

    return retval;
}

ret_val_t open_pid_sig_start_stop_close(int ctx_pid, int ctx_sig)
{
    pid_t pid = (pid_t)-1;
    int sig   = ctx_sig;
    sigset_t set;
    int signum;
    int handle = -1;
    int key    = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = ctx_pid;
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) +
              ", \"sig\": " + getStringFromNumber(sig) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {23, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {24, {0, 0}};
        return retval;
    }
    if (parsed.hasKey("sig"))
    {
        if (sig != parsed["sig"].asNumber<int>())
        {
            retval = {25, {0, 0}};
            return retval;
        }
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startPreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {26, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 5;
    timeout.tv_nsec = 0;

    // test for 10 frames
    for (int i = 0; i < 10; i++)
    {
        std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
        signum = sigtimedwait(&set, NULL, &timeout);
        if (signum != -1)
        {
            std::cout << "received" << std::endl;
            // Read Shared Memeory here and do somothing
        }
        else
        {
            std::cout << "timeout (10sec)" << std::endl;
            std::cout << "server stopped notifying write event." << std::endl;
            std::cout << "app termination sequence starts ..." << std::endl;
            break;
        }
    }

    // stopPreview
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {27, {0, 0}};
        return retval;
    }

    // close
    command = cmd_close + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {28, {handle, pid}};
        return retval;
    }

    return retval;
}

ret_val_t close_with_pid(int handle, int pid)
{
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};
    std::string command;
    std::string output;

    command =
        cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {29, {handle, pid}};
        return retval;
    }

    return retval;
}

ret_val_t open_invalid_pid_close_invalid_pid(int dummy_pid, int dummy_sig)
{
    pid_t pid = (pid_t)-1;
    sigset_t set;
    int handle = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = 1000000; // invalid pid
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {30, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {31, {0, 0}};
        return retval;
    }
    if (SIGUSR1 != parsed["sig"].asNumber<int>())
    {
        retval = {32, {0, 0}};
        return retval;
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // close
    command =
        cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {33, {handle, pid}};
        return retval;
    }

    return retval;
}

ret_val_t open_invalid_pid_start_stop_close_invalid_pid(int dummy_pid, int dummy_sig)
{
    pid_t pid = (pid_t)-1;
    sigset_t set;
    int signum;
    int handle = -1;
    int key    = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = 1000000; // invalid pid
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {34, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {35, {0, 0}};
        return retval;
    }
    if (SIGUSR1 != parsed["sig"].asNumber<int>())
    {
        retval = {36, {0, 0}};
        return retval;
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startPreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {37, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 5;
    timeout.tv_nsec = 0;

    // test for 10 frames
    for (int i = 0; i < 10; i++)
    {
        std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
        signum = sigtimedwait(&set, NULL, &timeout);
        if (signum != -1)
        {
            std::cout << "received" << std::endl;
            // Read Shared Memeory here and do somothing
        }
        else
        {
            std::cout << "timeout (10sec)" << std::endl;
            std::cout << "server stopped notifying write event." << std::endl;
            std::cout << "app termination sequence starts ..." << std::endl;
            break;
        }
    }

    // stopPreview
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {38, {0, 0}};
        return retval;
    }

    // close
    command =
        cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {39, {0, 0}};
        return retval;
    }

    return retval;
}

ret_val_t open_invalid_pid_start_stop_close(int dummy_pid, int dummy_sig)
{
    pid_t pid = (pid_t)-1;
    sigset_t set;
    int signum;
    int handle = -1;
    int key    = -1;
    std::string command;
    std::string output;
    pbnjson::JValue parsed;
    ret_val_t retval = {0, {0, 0}};

    // Block signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // open API
    // pid = getpid();
    pid     = 1000000; // invalid pid
    command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) + " }'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {40, {0, 0}};
        return retval;
    }
    if (pid != parsed["pid"].asNumber<int>())
    {
        retval = {41, {0, 0}};
        return retval;
    }
    if (SIGUSR1 != parsed["sig"].asNumber<int>())
    {
        retval = {42, {0, 0}};
        return retval;
    }
    handle = parsed["handle"].asNumber<int>();

    // setformat
    command = cmd_setformat + getStringFromNumber(handle) +
              ",\"params\":{\"width\":" + getStringFromNumber(640) +
              ",\"height\":" + getStringFromNumber(480) + ",\"format\": \"JPEG\"" +
              ",\"fps\":" + getStringFromNumber(30) + "}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;

    // startPreview
    command = cmd_startpreview + getStringFromNumber(handle) +
              ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {43, {0, 0}};
        return retval;
    }
    key = parsed["key"].asNumber<int>();
    (void)key;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 5;
    timeout.tv_nsec = 0;

    // test for 10 frames
    for (int i = 0; i < 10; i++)
    {
        std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
        signum = sigtimedwait(&set, NULL, &timeout);
        if (signum != -1)
        {
            std::cout << "received" << std::endl;
            // Read Shared Memeory here and do somothing
        }
        else
        {
            std::cout << "timeout (10sec)" << std::endl;
            std::cout << "server stopped notifying write event." << std::endl;
            std::cout << "app termination sequence starts ..." << std::endl;
            break;
        }
    }

    // stopPreview
    command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {44, {0, 0}};
        return retval;
    }

    // close
    command = cmd_close + getStringFromNumber(handle) + "}'";
    std::cout << command << std::endl;
    output = getCommandOutput(command);
    std::cout << output << std::endl;
    parsed = convertStringToJson(output.c_str());
    if (parsed["returnValue"].asBool() == false)
    {
        retval = {45, {0, 0}};
        return retval;
    }

    return retval;
}