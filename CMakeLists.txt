# @@@LICENSE
#
# Copyright (c) 2013 LG Electronics, Inc.
#
# LICENSE@@@

cmake_minimum_required(VERSION 2.8.7)
project(com.webos.service.camera CXX)
set(CMAKE_BUILD_TYPE Debug)

include(webOS/webOS)
webos_modules_init(1 6 3)

webos_component(1 0 0)

include(FindPkgConfig)


include_directories(inc)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LUNASERVICE2 REQUIRED luna-service2)
include_directories(${LUNASERVICE2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE2_CFLAGS_OTHER})

pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
include_directories(${PMLOGLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOGLIB_CFLAGS_OTHER})

pkg_check_modules(JSONC REQUIRED json-c)
include_directories(${JSONC_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${JSONC_CFLAGS})

pkg_check_modules(UDEV REQUIRED libudev)
include_directories(${UDEV_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${UDEV_CFLAGS})

pkg_check_modules(ALSA REQUIRED alsa)
include_directories(${ALSA_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${ALSA_CFLAGS})

webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

include_directories(${CMAKE_SOURCE_DIR})


#hal
include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/v4l2/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/alsa/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/utils/)

#service
include_directories(${CMAKE_SOURCE_DIR}/src/service/)
include_directories(${CMAKE_SOURCE_DIR}/files/)
include_directories(${CMAKE_SOURCE_DIR}/files/launch/)
include_directories(${CMAKE_SOURCE_DIR}/files/sysbus/)

include_directories(${CMAKE_SOURCE_DIR}/files/systemd/)

set(PROJECT_NAME_SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/hal/hal.cpp
    ${CMAKE_SOURCE_DIR}/src/hal/utils/camshm.cpp
    ${CMAKE_SOURCE_DIR}/src/hal/v4l2/hal_v4l2.cpp
    ${CMAKE_SOURCE_DIR}/src/hal/alsa/hal_alsa.cpp
    ${CMAKE_SOURCE_DIR}/src/service/camera_types.cpp
    ${CMAKE_SOURCE_DIR}/src/service/service_main.cpp
    ${CMAKE_SOURCE_DIR}/src/service/command_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/service/device_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/service/device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/service/service_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/service/command_service_util.cpp
    )

add_executable(${PROJECT_NAME} ${PROJECT_NAME_SRC_LIST})

target_link_libraries(${PROJECT_NAME}
    ${PMLOGLIB_LDFLAGS}
    ${JSONC_LDFLAGS}
    ${ALSA_LDFLAGS}
    ${GLIB2_LDFLAGS}
    ${LUNASERVICE2_LDFLAGS}
    ${UDEV_LDFLAGS}
    pthread)

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

webos_build_system_bus_files()
