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

project(notifier-pdm CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# TODO : Remove this after code unification
if(${WEBOS_TARGET_MACHINE} STREQUAL "raspberrypi4" OR ${WEBOS_TARGET_MACHINE} STREQUAL "raspberrypi4-64" OR ${WEBOS_TARGET_MACHINE} STREQUAL "qemux86" OR ${WEBOS_TARGET_MACHINE} STREQUAL "qemux86-64")
    add_definitions(-DPLATFORM_OSE)
endif()

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/pdm_client.cpp)

add_library(${PROJECT_NAME} SHARED ${SRC})

target_link_libraries(${PROJECT_NAME}
                      ${PMLOGLIB_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      camera_common
                      )

set_target_properties (${PROJECT_NAME} PROPERTIES VERSION 1.0 SOVERSION 1)

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_LIBDIR}/camera)
