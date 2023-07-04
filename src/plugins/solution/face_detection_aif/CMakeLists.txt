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

project(solution-facedetection-aif CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# find needed packages
find_package(PkgConfig)
pkg_check_modules(OPENCV REQUIRED opencv4)

# include directories
include_directories(
    ${OPENCV_INCLUDE_DIRS}
)

# link directories
link_directories(
    ${OPENCV_LIBRARY_DIRS}
)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/face_detection_aif.cpp)

add_library(${PROJECT_NAME} SHARED ${SRC})

target_link_libraries(${PROJECT_NAME}
                      ${PMLOGLIB_LDFLAGS}
                      camera_solution
                      camera_common
                      ${JPEG_LDFLAGS}
                      ${OPENCV_LIBRARIES}
                      edgeai-vision
                      ${LS2++_LDFLAGS}
                      )

set_target_properties (${PROJECT_NAME} PROPERTIES VERSION 1.0 SOVERSION 1)

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_LIBDIR}/camera)
