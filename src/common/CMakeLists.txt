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

cmake_minimum_required(VERSION 3.14)

include(webOS/webOS)
webos_modules_init(1 6 3)
webos_component(1 0 0)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS}) # glib.h
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

include_directories(../../include/public/camera)
include_directories(../../include/public/camera/hal) # camera_hal_types.h
include_directories(../../include/private)

include_directories(${CMAKE_SOURCE_DIR}/include/public/camera)

add_subdirectory(lib)
