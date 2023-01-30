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
project(com.webos.service.camera2.solution CXX)

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

pkg_check_modules(JPEG REQUIRED libjpeg)
include_directories(${JPEG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${JPEG_CFLAGS})

include_directories(${CMAKE_SOURCE_DIR}/src/plugins/solution/face_detection_aif)

include_directories(${CMAKE_SOURCE_DIR}/src/services/control)
include_directories(${CMAKE_SOURCE_DIR}/src/services/control/hal)
include_directories(${CMAKE_SOURCE_DIR}/src/services/solution)

# Camera Solution Service
set (SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/services/solution/service/CameraSolutionService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/solution/camera_solution.cpp
    ${CMAKE_SOURCE_DIR}/src/services/solution/camera_solution_async.cpp
    ${CMAKE_SOURCE_DIR}/src/services/control/camshm.cpp
    )

# plugins/solution
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/plugins/solution/face_detection_aif/face_detection_aif.cpp
    )

### SOLUTION MANAGER Add AI Framework Start ###
# find needed packages
find_package(PkgConfig)
pkg_check_modules(TFLITE REQUIRED tensorflowlite)
pkg_check_modules(OPENCV REQUIRED opencv4)
pkg_check_modules(RAPIDJSON REQUIRED RapidJSON)

# include directories
include_directories(
    ${TFLITE_INCLUDE_DIRS}
    ${OPENCV_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIRS}
)

# link directories
link_directories(
    ${TFLITE_LIBRARY_DIRS}
    ${OPENCV_LIBRARY_DIRS}
    ${RAPIDJSON_LIBRARY_DIRS}
)
### SOLUTION MANAGER Add AI Framework End ###

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS}
                      ${JPEG_LDFLAGS}
                      ${OPENCV_LIBRARIES}
                      edgeai-vision
                      pthread
                      )

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

add_subdirectory(files)
