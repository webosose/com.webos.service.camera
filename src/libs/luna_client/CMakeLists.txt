# Copyright (c) 2023 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

project(luna_client CXX)

pkg_check_modules(LS2++ REQUIRED luna-service2++>=3)
include_directories(${LS2++_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2++_CFLAGS})

include_directories(${CMAKE_SOURCE_DIR}/include/public/camera)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/luna_client.cpp
    )

add_library(${PROJECT_NAME} SHARED ${SRC})

target_link_libraries(${PROJECT_NAME}
                      ${PMLOGLIB_LDFLAGS}
                      ${GLIB2_LDFLAGS}
                      ${LS2++_LDFLAGS}
                     )

set_target_properties (${PROJECT_NAME} PROPERTIES VERSION 1.0 SOVERSION 1)

webos_build_library(NAME ${PROJECT_NAME})
