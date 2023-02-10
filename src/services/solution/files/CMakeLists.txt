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

configure_file(sysbus/com.webos.camerasolution.role.json.in sysbus/com.webos.camerasolution.role.json @ONLY)
configure_file(sysbus/com.webos.camerasolution.service.in sysbus/com.webos.camerasolution.service @ONLY)
configure_file(sysbus/com.webos.camerasolution.api.json sysbus/com.webos.camerasolution.api.json @ONLY)
configure_file(sysbus/com.webos.camerasolution.perm.json sysbus/com.webos.camerasolution.perm.json @ONLY)
configure_file(sysbus/com.webos.camerasolution.groups.json sysbus/com.webos.camerasolution.groups.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.camerasolution.role.json
    DESTINATION /usr/share/luna-service2/roles.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.camerasolution.service
    DESTINATION /usr/share/luna-service2/services.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.camerasolution.api.json
    DESTINATION /usr/share/luna-service2/api-permissions.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.camerasolution.perm.json
    DESTINATION /usr/share/luna-service2/client-permissions.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.camerasolution.groups.json
    DESTINATION /usr/share/luna-service2/groups.d/)
