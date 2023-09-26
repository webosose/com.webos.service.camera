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

#define LOG_TAG "Process"
#include "process.h"
#include "camera_log.h"
#include <iterator>
#include <sstream>
#include <sys/wait.h>
#include <vector>

Process::Process(const std::string &cmd)
{
    PLOGI("");

    start(cmd);
}

Process::~Process()
{
    PLOGI("");

    stop();
}

void Process::start(const std::string &cmd)
{
    PLOGI("%s", cmd.c_str());

    _pid = fork();
    if (_pid == 0)
    {
        std::istringstream iss(cmd);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>{}};
        size_t tokens_size = (tokens.size() < (SIZE_MAX)) ? tokens.size() + 1 : tokens.size();
        char **argv = new char *[(tokens_size < (SIZE_MAX / sizeof(char *))) ? tokens_size : 0];
        size_t v    = 0;
        for (const auto &token : tokens)
        {
            size_t token_size = (token.size() < SIZE_MAX) ? token.size() + 1 : token.size();
            argv[v]           = new char[token_size];
            strncpy(argv[(v < SIZE_MAX) ? v++ : v], token.c_str(), token_size);
        }
        argv[v] = nullptr;
        execv(argv[0], argv);
        _exit(0);
    }
}
void Process::stop()
{
    PLOGI("pid %d", _pid);

    int status    = 0;
    pid_t waitPid = wait(&status);
    if (waitPid == -1)
    {
        PLOGE("error : %d", errno);
    }
    else
    {
        if (WIFEXITED(status))
        {
            PLOGI("normal exit status %d", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            PLOGI("abnormal exit status %d", WTERMSIG(status));
        }
    }

    PLOGI("end pid %d", waitPid);
}