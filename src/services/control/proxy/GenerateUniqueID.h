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

#ifndef GENERATE_UNIQUE_ID_H_
#define GENERATE_UNIQUE_ID_H_

#include <functional>
#include <random>
#include <string>
#include <time.h>

#define DEVICECONTROL_UNIQUE_ID_LENGTH 15

class GenerateUniqueID
{
    const std::string source_;
    const int base_;
    const std::function<int()> rand_;

public:
    explicit GenerateUniqueID(
        const std::string &src = "0123456789ABCDEFGIJKLMNOPQRSTUVWXYZabcdefgijklmnopqrstuvwxyz")
        : source_(src), base_(source_.size()),
          rand_(std::bind(std::uniform_int_distribution<int>(0, base_ - 1),
                          std::mt19937(std::random_device()())))
    {
    }

    std::string operator()()
    {
        struct timespec time;
        std::string s(DEVICECONTROL_UNIQUE_ID_LENGTH, '0');

        clock_gettime(CLOCK_MONOTONIC, &time);

        s[0] = '_'; // Prepend uid with _ to comply with luna requirements
        for (int i = 1; i < DEVICECONTROL_UNIQUE_ID_LENGTH; ++i)
        {
            if (i < 5 && i < DEVICECONTROL_UNIQUE_ID_LENGTH - 6)
            {
                s[i] = source_[time.tv_nsec % base_];
                time.tv_nsec /= base_;
            }
            else if (time.tv_sec > 0 && i < DEVICECONTROL_UNIQUE_ID_LENGTH - 3)
            {
                s[i] = source_[time.tv_sec % base_];
                time.tv_sec /= base_;
            }
            else
            {
                s[i] = source_[rand_()];
            }
        }

        return s;
    }
};

#endif /* GENERATE_UNIQUE_ID_H_ */
