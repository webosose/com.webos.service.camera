// Copyright (c) 2023 LG Electronics, Inc.
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

#include "process.h"
#include "camera_types.h"
#include <iterator>
#include <sstream>
#include <sys/wait.h>
#include <vector>

Process::Process(const std::string &cmd)
{
    PMLOG_INFO(CONST_MODULE_PR, "");

    start(cmd);
}

Process::~Process()
{
    PMLOG_INFO(CONST_MODULE_PR, "");

    stop();
}

void Process::start(const std::string &cmd)
{
    PMLOG_INFO(CONST_MODULE_PR, "%s", cmd.c_str());

    _pid = fork();
    if (_pid == 0)
    {
        std::istringstream iss(cmd);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>{}};
        char **argv = new char *[tokens.size() + 1];
        size_t v    = 0;
        for (const auto &token : tokens)
        {
            argv[v] = new char[token.size() + 1];
            strncpy(argv[v++], token.c_str(), token.size() + 1);
        }
        argv[v] = nullptr;
        execv(argv[0], argv);
        _exit(0);
    }
}
void Process::stop()
{
    PMLOG_INFO(CONST_MODULE_PR, "pid %d", _pid);

    int status    = 0;
    pid_t waitPid = wait(&status);
    if (waitPid == -1)
    {
        PMLOG_ERROR(CONST_MODULE_PR, "error : %d", errno);
    }
    else
    {
        if (WIFEXITED(status))
        {
            PMLOG_INFO(CONST_MODULE_PR, "normal exit status %d", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            PMLOG_INFO(CONST_MODULE_PR, "abnormal exit status %d", WTERMSIG(status));
        }
    }

    PMLOG_INFO(CONST_MODULE_PR, "end pid %d", waitPid);
}