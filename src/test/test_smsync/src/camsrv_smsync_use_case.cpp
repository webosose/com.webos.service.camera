#include "camsrv_smsync_use_case.h"
#include "cmdpipeio.h"
#include "jsonparse.h"
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <signal.h>


std::string cmd_getcameralist = "luna-send -n 1 -f luna://com.webos.service.camera2/getCameraList '{}'";
std::string cmd_getinfo = "luna-send -n 1 -f luna://com.webos.service.camera2/getInfo '{\"id\": \"";
std::string cmd_open = "luna-send -n 1 -f luna://com.webos.service.camera2/open '{\"id\": \"";
std::string cmd_setformat = "luna-send -n 1 -f luna://com.webos.service.camera2/setFormat '{\"handle\":";
std::string cmd_startpreview = "luna-send -n 1 -f luna://com.webos.service.camera2/startPreview '{\"handle\":";
std::string cmd_stoppreview = "luna-send -n 1 -f luna://com.webos.service.camera2/stopPreview '{\"handle\":";
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
  int handle = -1;
  int key = -1;
  std::string command;
  std::string output;
  pbnjson::JValue parsed;
  ret_val_t retval = {0, 0};

  // open
  command = cmd_open +"camera1\"}'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl;
  parsed = convertStringToJson(output.c_str());
  bRetVal = parsed["returnValue"].asBool();
  if (bRetVal == false)
  {
     retval = {1, 0};
     return retval;
  }
  handle = parsed["handle"].asNumber<int>();
  
  // setformat
  command = cmd_setformat + getStringFromNumber(handle) +
                ",\"params\":{\"width\":" + getStringFromNumber(640) +
                ",\"height\":" + getStringFromNumber(480) +
	           	",\"format\": \"JPEG\"" +
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
  parsed = convertStringToJson(output.c_str());
  bRetVal = parsed["returnValue"].asBool();
  if (bRetVal == false)
  {
     retval = {2, 0};
     return retval;
  }
  key = parsed["key"].asNumber<int>();
 
  // Do something
  for (int i = 0; i < 10; i++)
  { 
      std::cout << i <<": app is doing something ... " << std::endl;
      sleep(1);
  }


  // stoppreviewa
  command = cmd_stoppreview + getStringFromNumber(handle) + "}'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl;
  parsed = convertStringToJson(output.c_str());
  bRetVal = parsed["returnValue"].asBool();
  if (bRetVal == false)
  {
     retval = {3, 0};
     return retval;
  }
   
  // close
  command = cmd_close + getStringFromNumber(handle) + "}'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl;
  parsed = convertStringToJson(output.c_str());
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
  pid_t pid=(pid_t)-1;
  sigset_t set;
  int signum;
  int handle = -1;
  int key = -1;
  std::string command;
  std::string output;
  pbnjson::JValue parsed;
  ret_val_t retval = {0, 0};
  
  // Block signal
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_SETMASK, &set, NULL);


  // open API
  //pid = getpid();
  pid = ctx_pid;
  command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) + " }'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl; 
  parsed = convertStringToJson(output.c_str());
  if (parsed["returnValue"].asBool() == false)
  {
      retval = {5, 0};
      return retval;
  }
  if (pid != parsed["pid"].asNumber<int>())
  {
      retval = {6, 0};
      return retval;
  }
  if (SIGUSR1 != parsed["sig"].asNumber<int>())
  {
      retval = {7, 0};
      return retval;
  }
  handle = parsed["handle"].asNumber<int>();

  // setformat
  command = cmd_setformat + getStringFromNumber(handle) +
                ",\"params\":{\"width\":" + getStringFromNumber(640) +
                ",\"height\":" + getStringFromNumber(480) +
                ",\"format\": \"JPEG\"" +
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
     retval = {8, 0};
     return retval;
  }
  key = parsed["key"].asNumber<int>();

  // start of synchronization test
  struct timespec timeout;
  timeout.tv_sec = 5;
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
      retval = {9, 0};
      return retval;
  }

  // close
  command = cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl;
  parsed = convertStringToJson(output.c_str());
  if (parsed["returnValue"].asBool() == false)
  {
     if (0 == parsed["errorCode"].asNumber<int>())
     {
         retval = {-1, handle};
         return retval;
     }
     retval = {10, 0};
     return retval;
  }

  return retval;
}


ret_val_t open_pid_sig_start_stop_close_pid(int ctx_pid, int ctx_sig)
{
  pid_t pid=(pid_t)-1;
  int sig = ctx_sig;
  sigset_t set;
  int signum;
  int handle = -1;
  int key = -1;
  std::string command;
  std::string output;
  pbnjson::JValue parsed;
  ret_val_t retval = {0, 0};
  
  // Block signal
  sigemptyset(&set);
  sigaddset(&set, sig);
  sigprocmask(SIG_SETMASK, &set, NULL);


  // open API
  //pid = getpid();
  pid = ctx_pid;
  command = cmd_open + "camera1\", \"pid\": " + getStringFromNumber(pid) 
                           + ", \"sig\": " + getStringFromNumber(sig) + " }'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl; 
  parsed = convertStringToJson(output.c_str());
  if (parsed["returnValue"].asBool() == false)
  {
      retval = {11, 0};
      return retval;
  }
  if (pid != parsed["pid"].asNumber<int>())
  {
      retval = {12, 0};
      return retval;
  }
  if (parsed.hasKey("sig"))
  {
      if (sig != parsed["sig"].asNumber<int>())
      {
          retval = {13, 0};
          return retval;
      }
  }
  handle = parsed["handle"].asNumber<int>();

  // setformat
  command = cmd_setformat + getStringFromNumber(handle) +
                ",\"params\":{\"width\":" + getStringFromNumber(640) +
                ",\"height\":" + getStringFromNumber(480) +
                ",\"format\": \"JPEG\"" +
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
     retval = {14, 0};
     return retval;
  }
  key = parsed["key"].asNumber<int>();

  // start of synchronization test
  struct timespec timeout;
  timeout.tv_sec = 5;
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
      retval = {15, 0};
      return retval;
  }

  // close
  command = cmd_close + getStringFromNumber(handle) + ", \"pid\": " + getStringFromNumber(pid) + "}'";
  std::cout << command << std::endl;
  output = getCommandOutput(command);
  std::cout << output << std::endl;
  parsed = convertStringToJson(output.c_str());
  if (parsed["returnValue"].asBool() == false)
  {
     if (0 == parsed["errorCode"].asNumber<int>())
     {
         retval = {16, handle};
         return retval;
     }
     retval = {17, 0};
     return retval;
  }


  return retval;
}


ret_val_t close_exception_handler(int dummy_pid, int handle)
{
   ret_val_t retval = {0, 0};
   std::string command = cmd_close + getStringFromNumber(handle) + "}'";
   std::cout << command << std::endl;
   std::string output = getCommandOutput(command);
   std::cout << output << std::endl;
   pbnjson::JValue parsed = convertStringToJson(output.c_str());
   if (parsed["returnValue"].asBool() == false)
   {
      retval = {18, 0};
      return retval;
   }
   return retval;
}



