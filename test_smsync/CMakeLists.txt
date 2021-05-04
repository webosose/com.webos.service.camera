# Copyright (c) 2020 LG Electronics, Inc.
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
project(camera_service_shared_memory_sync_test CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})


set(BIN_NAME camsrv_smsync_test)

set(SRC_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/camsrv_smsync_test_main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/camsrv_smsync_use_case_runner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/camsrv_smsync_use_case.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cmdpipeio.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jsonparse.cpp
)

add_executable(${BIN_NAME} ${SRC_LIST})
add_compile_options( --std=c++11 )
set_target_properties(${BIN_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries (${BIN_NAME}
    ${PBNJSON_CPP_LDFLAGS}
)


install(TARGETS ${BIN_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})
