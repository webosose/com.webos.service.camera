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

project(com.webos.service.camera2 CXX)

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS})

include_directories(${CMAKE_SOURCE_DIR}/src/plugins/notifier/appcast)
include_directories(${CMAKE_SOURCE_DIR}/src/plugins/notifier/pdm)

#service
include_directories(${CMAKE_SOURCE_DIR}/src/services/camera)
include_directories(${CMAKE_SOURCE_DIR}/src/services/camera/addon)
include_directories(${CMAKE_SOURCE_DIR}/src/services/camera/notifier)

# Camera service
set(SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/services/camera/camera_service.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/command_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/virtual_device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/json_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/whitelist_checker.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/event_notification.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/addon/addon.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/notifier/notifier.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/CameraHalProxy.cpp
    )

# common
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/services/common/camera_types.cpp
    ${CMAKE_SOURCE_DIR}/src/services/common/Process.cpp
    )

add_executable(${PROJECT_NAME}
               ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS}
                      pthread
                      ${CMAKE_DL_LIBS}
                      camera_common
                      )

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})
