# Copyright (c) 2024 LG Electronics, Inc.
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

if(USE_NEW_ACG)
    message("USE_NEW_ACG is ${USE_NEW_ACG}")
    set(SYSBUS "sysbus_new")
else()
    set(SYSBUS "sysbus")
endif()

configure_file(${SYSBUS}/camerahal.manifest.json.in ${SYSBUS}/camerahal.manifest.json @ONLY)
configure_file(${SYSBUS}/com.webos.camerahal.role.json.in ${SYSBUS}/com.webos.camerahal.role.json @ONLY)
configure_file(${SYSBUS}/com.webos.camerahal.service.in ${SYSBUS}/com.webos.camerahal.service @ONLY)
configure_file(${SYSBUS}/com.webos.camerahal.api.json ${SYSBUS}/com.webos.camerahal.api.json @ONLY)
configure_file(${SYSBUS}/com.webos.camerahal.perm.json ${SYSBUS}/com.webos.camerahal.perm.json @ONLY)
configure_file(${SYSBUS}/com.webos.camerahal.groups.json ${SYSBUS}/com.webos.camerahal.groups.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SYSBUS}/camerahal.manifest.json
    DESTINATION /usr/share/luna-service2/manifests.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SYSBUS}/com.webos.camerahal.role.json
    DESTINATION /usr/share/luna-service2/roles.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SYSBUS}/com.webos.camerahal.service
    DESTINATION /usr/share/luna-service2/services.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SYSBUS}/com.webos.camerahal.api.json
    DESTINATION /usr/share/luna-service2/api-permissions.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SYSBUS}/com.webos.camerahal.perm.json
    DESTINATION /usr/share/luna-service2/client-permissions.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SYSBUS}/com.webos.camerahal.groups.json
    DESTINATION /usr/share/luna-service2/groups.d/)
