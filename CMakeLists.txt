# Copyright (c) 2019 LG Electronics, Inc.
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
project(com.webos.service.camera2 CXX)
set(CMAKE_BUILD_TYPE Debug)

include(webOS/webOS)
webos_modules_init(1 6 3)

webos_component(1 0 0)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LS2++ REQUIRED luna-service2++>=3)
include_directories(${LS2++_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2++_CFLAGS})

pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
include_directories(${PMLOGLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOGLIB_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS})

pkg_check_modules(UDEV REQUIRED libudev)
include_directories(${UDEV_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${UDEV_CFLAGS})

if(WEBOS_TARGET_MACHINE STREQUAL "raspberrypi3")
add_definitions(-DPLATFORM_RASPBERRYPI3)
endif()

if(WEBOS_TARGET_MACHINE STREQUAL "raspberrypi4")
add_definitions(-DPLATFORM_RASPBERRYPI4)
endif()

if(WEBOS_TARGET_MACHINE STREQUAL "qemux86")
add_definitions(-DPLATFORM_QEMUX86)
endif()

webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Werror=return-type")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/files/)
include_directories(${CMAKE_SOURCE_DIR}/files/sysbus/)
include_directories(${CMAKE_SOURCE_DIR}/files/systemd/)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

#service
include_directories(${CMAKE_SOURCE_DIR}/src/service/)
include_directories(${CMAKE_SOURCE_DIR}/src/service/notifier/)
include_directories(${CMAKE_SOURCE_DIR}/src/service/notifier/pdm/)

set(SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/service/camera_service.cpp
    ${CMAKE_SOURCE_DIR}/src/service/command_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/service/device_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/service/virtual_device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/service/device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/service/json_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/service/notifier/notifier.cpp
    ${CMAKE_SOURCE_DIR}/src/service/notifier/pdm/pdm_client.cpp
    ${CMAKE_SOURCE_DIR}/src/service/camshm.cpp
    ${CMAKE_SOURCE_DIR}/src/service/camera_types.cpp
    )

add_executable(${PROJECT_NAME}
               ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS}
                      ${UDEV_LDFLAGS}
                      pthread
                      camera_hal)

#hal
include_directories(${CMAKE_SOURCE_DIR}/src/hal/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/v4l2)

install(FILES ${CMAKE_SOURCE_DIR}/src/include/camera_hal_if.h DESTINATION
${WEBOS_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_SOURCE_DIR}/src/include/camera_hal_types.h DESTINATION
${WEBOS_INSTALL_INCLUDEDIR})

#cameraservice project exe
set (SRC_CAMSRV_TEST ${CMAKE_SOURCE_DIR}/src/test/cameraservice/camera_service_test.cpp)
add_executable (camsrv_test_app ${SRC_CAMSRV_TEST})
install(TARGETS camsrv_test_app DESTINATION ${WEBOS_INSTALL_SBINDIR})

#libcamera_hal.so
set (SRC_HAL
     ${CMAKE_SOURCE_DIR}/src/hal/camera_base_wrapper.cpp
     ${CMAKE_SOURCE_DIR}/src/hal/camera_hal_if.cpp)
add_library(camera_hal SHARED ${SRC_HAL})
set_target_properties (camera_hal PROPERTIES VERSION 1.0 SOVERSION 1)

#v4l2-camera-plugin library
set (SRC_V4L2 ${CMAKE_SOURCE_DIR}/src/hal/v4l2/v4l2_camera_plugin.cpp)
add_library(v4l2-camera-plugin SHARED ${SRC_V4L2})
set_target_properties (v4l2-camera-plugin  PROPERTIES VERSION 1.0 SOVERSION 1)

#link libraries camera_hal and v4l2-camera-plugin
target_link_libraries(camera_hal
                      v4l2-camera-plugin
                      ${CMAKE_DL_LIBS}
                      ${PMLOGLIB_LDFLAGS}
                      pthread)
install(TARGETS v4l2-camera-plugin DESTINATION ${WEBOS_INSTALL_LIBDIR})
install(TARGETS camera_hal DESTINATION ${WEBOS_INSTALL_LIBDIR})

#haltestapp project exe
set (SRC_CAMHAL_TEST ${CMAKE_SOURCE_DIR}/src/test/hal/camera_hal_tests.cpp)
add_executable (hal_test_app ${SRC_CAMHAL_TEST})
target_link_libraries (hal_test_app
                       camera_hal)
install(TARGETS hal_test_app DESTINATION ${WEBOS_INSTALL_SBINDIR})

set(HAL_GTEST TRUE CACHE BOOL "Set to TRUE to enable gtest compilation for HAL")
if (HAL_GTEST)
    webos_use_gtest()
    include_directories(${WEBOS_GTEST_INCLUDES})
    add_subdirectory(src/gtest)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})
webos_build_system_bus_files()
