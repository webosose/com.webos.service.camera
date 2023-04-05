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

#pragma once

#include "pbnjson.h"
#include <nlohmann/json.hpp>
#include <optional>

static inline int deSerialize(const char *strInput, const char *inputSchema, jvalue_ref &objJson)
{
    jerror *error      = NULL;
    jschema_ref schema = jschema_create(j_cstr_to_buffer(inputSchema), &error);

    if (error)
    {
        jerror_free(error);
        return -1;
    }

    objJson = jdom_create(j_cstr_to_buffer(strInput), schema, &error);

    jschema_release(&schema);
    if (error)
    {
        jerror_free(error);
        return -1;
    }

    return 0;
}

using namespace nlohmann;

template <typename T>
inline std::optional<T> get_optional(const json &j, const char *key)
{
    try
    {
        std::optional<T> value;
        const auto it = j.find(key);
        if (it != j.end())
        {
            value = it->template get<T>();
        }
        else
        {
            value = std::nullopt;
        }
        return value;
    }
    catch (json::exception &e)
    {
        return std::nullopt;
    }
    catch (const std::logic_error &e)
    {
        return std::nullopt;
    }
}
