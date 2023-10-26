# Copyright (c) 2019-2023 LG Electronics, Inc.
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
project(com.webos.service.camera2 CXX)
set(CMAKE_BUILD_TYPE Debug)

include(webOS/webOS)
webos_modules_init(1 6 3)
webos_component(1 0 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lrt -Werror -Wno-psabi")

# Use WEBOS_INSTALL_LIBDIR instead of the hardcoded path
add_compile_definitions(WEBOS_INSTALL_LIBDIR="${WEBOS_INSTALL_LIBDIR}")
configure_file(${CMAKE_SOURCE_DIR}/include/public/camera/plugin/plugin_factory.hpp.in ${CMAKE_SOURCE_DIR}/include/public/camera/plugin/plugin_factory.hpp @ONLY)

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

include_directories(${CMAKE_SOURCE_DIR}/include/public)
include_directories(${CMAKE_SOURCE_DIR}/include/public/camera)
include_directories(${CMAKE_SOURCE_DIR}/include/public/camera/plugin)
include_directories(${CMAKE_SOURCE_DIR}/include/public/camera/hal)
include_directories(${CMAKE_SOURCE_DIR}/include/public/camera/notifier)
include_directories(${CMAKE_SOURCE_DIR}/include/public/camera/solution)
include_directories(${CMAKE_SOURCE_DIR}/include/private)

webos_use_gtest()

add_subdirectory(src/common)
add_subdirectory(src/plugins)
add_subdirectory(src/services)
add_subdirectory(tests)

webos_build_system_bus_files()
webos_build_configured_file(files/systemd/com.webos.service.camera.service SYSCONFDIR systemd/system)
webos_build_configured_file(files/systemd/camera-registry.service SYSCONFDIR systemd/system)
webos_build_configured_file(files/systemd/camera-registry.sh SYSCONFDIR systemd/system/scripts)
