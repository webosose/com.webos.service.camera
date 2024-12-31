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

cmake_minimum_required(VERSION 2.8.7)
project(com.webos.service.camera2.hal CXX)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LUNASERVICE2 REQUIRED luna-service2)
include_directories(${LUNASERVICE2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE2_CFLAGS_OTHER})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})

pkg_check_modules(GST REQUIRED gstreamer-1.0)
include_directories(${GST_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/src/services/hal)
include_directories(${CMAKE_SOURCE_DIR}/src/services/hal/hal_if)

# Camera Hal Service
set(SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/services/hal/camera_hal_service.cpp
    ${CMAKE_SOURCE_DIR}/src/services/hal/device_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/services/hal/camera_solution_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/hal/camera_solution_proxy.cpp
    ${CMAKE_SOURCE_DIR}/src/services/hal/storage_monitor.cpp
    )

# common
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/services/common/camera_types.cpp
    ${CMAKE_SOURCE_DIR}/src/services/common/process.cpp
    )

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS}
                      ${GST_LIBRARIES}
                      ${CMAKE_DL_LIBS}
                      camera_shared_memory
                      luna_client
                      pthread
                      )

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

#hal_if
add_subdirectory(hal_if)

add_subdirectory(files)
