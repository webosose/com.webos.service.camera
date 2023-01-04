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

pkg_check_modules(JPEG REQUIRED libjpeg)
include_directories(${JPEG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${JPEG_CFLAGS})

pkg_check_modules(GST REQUIRED gstreamer-1.0)
include_directories(${GST_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/src/plugins/notifier/appcast)
include_directories(${CMAKE_SOURCE_DIR}/src/plugins/notifier/pdm)
include_directories(${CMAKE_SOURCE_DIR}/src/plugins/solution/auto_contrast)
include_directories(${CMAKE_SOURCE_DIR}/src/plugins/solution/dummy)
include_directories(${CMAKE_SOURCE_DIR}/src/plugins/solution/face_detection_aif)

#service
include_directories(${CMAKE_SOURCE_DIR}/src/services/camera)
include_directories(${CMAKE_SOURCE_DIR}/src/services/camera/addon)
include_directories(${CMAKE_SOURCE_DIR}/src/services/camera/notifier)
include_directories(${CMAKE_SOURCE_DIR}/src/services/control)
include_directories(${CMAKE_SOURCE_DIR}/src/services/control/hal)
include_directories(${CMAKE_SOURCE_DIR}/src/services/solution)

# Camera service
set(SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/services/camera/camera_service.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/command_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/virtual_device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/json_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/camera_types.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/whitelist_checker.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/event_notification.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/addon/addon.cpp
    ${CMAKE_SOURCE_DIR}/src/services/camera/notifier/notifier.cpp
    )

# Control service
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/services/control/camshm.cpp
    ${CMAKE_SOURCE_DIR}/src/services/control/cam_posixshm.cpp
    ${CMAKE_SOURCE_DIR}/src/services/control/device_controller.cpp
    )

# Solutions Service
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/services/solution/camera_solution_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/solution/camera_solution.cpp
    ${CMAKE_SOURCE_DIR}/src/services/solution/camera_solution_async.cpp
    )

# plugins/notifier
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/plugins/notifier/pdm/pdm_client.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/notifier/appcast/appcast_client.cpp
    )

# plugins/solution
set (SRC_LIST
    ${SRC_LIST}
    ${CMAKE_SOURCE_DIR}/src/plugins/solution/dummy/dummy.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/solution/auto_contrast/auto_contrast.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/solution/face_detection_aif/face_detection_aif.cpp
    )


### SOLUTION MANAGER Solution configuration file Start ###
message(STATUS "Using json to define supported solutions for variable platform")
webos_build_configured_file(src/services/solution/supported_solution_info/supported_solution_info.conf SYSCONFDIR com.webos.service.camera)
### SOLUTION MANAGER Solution configuration file End ###

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

add_executable(${PROJECT_NAME}
               ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS}
                      pthread
                      camera_hal
                      ${JPEG_LDFLAGS}
                      ${TFLITE_LIBRARIES}
                      ${OPENCV_LIBRARIES}
                      ${RAPIDJSON_LIBRARIES}
                      ${GST_LIBRARIES}
                      edgeai-vision
                      )

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

#hal
add_subdirectory(control/hal)
