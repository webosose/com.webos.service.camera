# Copyright (c) 2019-2021 LG Electronics, Inc.
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

project(camera-plugin-inspect CXX)

set(SRC camera_plugin_inspect.cpp)

add_executable (${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME}
                      ${CMAKE_DL_LIBS}
                      ${GLIB2_LDFLAGS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_BINDIR})
