// Copyright (c) 2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <signal.h>
#include <iostream>
#include "camsrv_smsync_use_case_runner.h"
#include "camsrv_smsync_use_case.h"


#define NO_ERROR   0

#define MAX_TEST_CASES 100


bool tc_passed[MAX_TEST_CASES];

int tc_num = 0;


int main(int argc, char const *argv[])
{
  ret_val_t retval = {0, {0, 0}};


  // test usual case senario
  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_start_stop_close, 0);
  std::cout << "use case 1 -- convensional usage test (no sync): retval = {"
                     << retval.value << ", {" << retval.info[0] << ", "
                     << retval.info[1] << "}}" << std::endl;
  if (retval.value == NO_ERROR)
  {
      tc_passed[tc_num] = true;
  }
  else
  {
      tc_passed[tc_num] = false;
  }
  tc_num++;

  // test sync case senario 1
  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_pid_start_stop_close_pid, 0);
  std::cout << "use case 2 -- synchronous usage test with default signal SIGUSR1: retval = {"
                       << retval.value << ", {" << retval.info[0] << ", "
                       << retval.info[1] << "}}" << std::endl;
  if (retval.value == NO_ERROR)
  {
      tc_passed[tc_num] = true;
  }
  else
  {
      tc_passed[tc_num] = false;
  }
  tc_num++;

  // test_syn case senario 2
  for (int i = SIGHUP; i <= SIGSYS; i++)
  {
      if (i == SIGKILL || i == SIGSTOP) continue;
      std::cout << "\nTC #" << tc_num + 1 << std::endl;
      retval = run_as_child_process(open_pid_sig_start_stop_close_pid, i);
      std::cout << "use case 3 -- synchronous usage test with user specified signal: retval = {"
                        << retval.value << ", {" << retval.info[0] << ", "
                        << retval.info[1] << "}}" << std::endl;
      if (retval.value == NO_ERROR)
      {
         tc_passed[tc_num] = true;
      }
      else
      {
         tc_passed[tc_num] = false;
      }
      tc_num++;
  }

  // test logic for exception handling leading to safe closing the device.
  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_pid_sig_start_stop_close_pid, SIGKILL);
  std::cout << "use case 3 -- invalid signal: retval = {" << retval.value << ", " << retval.info << "}" << std::endl;
  if (retval.value != NO_ERROR)
  {
     tc_passed[tc_num] = true;
  }
  else
  {
     tc_passed[tc_num] = false;
  }
  tc_num++;

  // test logic for exception handling leading to safe closing the device.
  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_pid_sig_start_stop_close_pid, SIGSTOP);
  std::cout << "use case 3 -- invalid signal: retval = {" << retval.value << ", " << retval.info << "}" << std::endl;
  if (retval.value != NO_ERROR)
  {
     tc_passed[tc_num] = true;
  }
  else
  {
     tc_passed[tc_num] = false;
  }
  tc_num++;




  // test logic for exception handling leading to safe closing the device.
  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_pid_start_stop_close, 0);
  std::cout << "use case 4 -- handling close exception >> synchronous usage test with default signal SIGUSR1: retval = {"
                       << retval.value << ", {" << retval.info[0] << ", "
                       << retval.info[1] <<"}}" << std::endl;
  if (retval.value == NO_ERROR)
  {
      tc_passed[tc_num] = false;
  }
  else
  {
     if (retval.info[0] != 0 && retval.info[1] != 0)
     {
        retval = close_with_pid(retval.info[0], retval.info[1]);
        if (retval.value == NO_ERROR)
        {
           tc_passed[tc_num] = true;
        }
        else
        {
           tc_passed[tc_num] = false;
        }
     }
     else
     {
        tc_passed[tc_num] = false;
     }
  }
  tc_num++;


  // test logic for exception handling leading to safe closing the device.
  for (int i = SIGHUP; i <= SIGSYS; i++)
  {
      if (i == SIGKILL || i == SIGSTOP) continue;
      std::cout << "\nTC #" << tc_num + 1 << std::endl;
      retval = run_as_child_process(open_pid_sig_start_stop_close, i);
      std::cout << "use case 5 -- handling close exception >> synchronous usage test with user specified signal: retval = {"
                        << retval.value << ", {" << retval.info[0] << ", "
                        << retval.info[1] <<"}}" << std::endl;
      if (retval.value == NO_ERROR)
      {
         tc_passed[tc_num] = false;
      }
      else
      {
         if (retval.info[0] != 0 && retval.info[1] != 0)
         {
            retval = close_with_pid(retval.info[0], retval.info[1]);
            if (retval.value == NO_ERROR)
            {
               tc_passed[tc_num] = true;
            }
            else
            {
               tc_passed[tc_num] = false;
            }
         }
         else
         {
            tc_passed[tc_num] = false;
         }
      }
      tc_num++;
  }

  // test logic for exception handling with invalid pid
  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_invalid_pid_close_invalid_pid, 0);
  std::cout << "use case 6 -- open and close with imvalid pid with default signal SIGUSR1: retval = {"
                       << retval.value << ", {" << retval.info[0] << ", "
                       << retval.info[1] <<"}}" << std::endl;
  if (retval.value == NO_ERROR)
  {
      tc_passed[tc_num] = true;
  }
  else
  {
      tc_passed[tc_num] = false;
  }
  tc_num++;


  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_invalid_pid_start_stop_close_invalid_pid, 0);
  std::cout << "use case 7 -- open-startpreview-stoppreview-close with invalid pid with default signal SIGUSR1: retval = {"
                       << retval.value << ", {" << retval.info[0] << ", "
                       << retval.info[1] <<"}}" << std::endl;
  if (retval.value == NO_ERROR)
  {
      tc_passed[tc_num] = true;
  }
  else
  {
      tc_passed[tc_num] = false;
  }
  tc_num++;

  std::cout << "\nTC #" << tc_num + 1 << std::endl;
  retval = run_as_child_process(open_invalid_pid_start_stop_close, 0);
  std::cout << "use case 8 -- open with invalid pid-startpreview-stoppreview-close with default signal SIGUSR1: retval = {"
                       << retval.value << ", {" << retval.info[0] << ", "
                       << retval.info[1] <<"}}" << std::endl;
  if (retval.value == NO_ERROR)
  {
      tc_passed[tc_num] = true;
  }
  else
  {
      tc_passed[tc_num] = false;
  }
  tc_num++;


  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Test Reports:" << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  for (int i = 0; i < tc_num; i++)
  {
      std::cout << "Test Case #" << i + 1 << ": ";
      if (tc_passed[i])
      {
         std::cout << "PASS" << std::endl;
      }
      else
      {
         std::cout << "FAIL" << std::endl;
      }
  }
  std::cout << "-----------------------------------------------" << std::endl;

  return 0;
}
