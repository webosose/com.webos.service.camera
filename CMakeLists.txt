# @@@LICENSE
#
# Copyright (c) 2013 LG Electronics, Inc.
#
# LICENSE@@@

cmake_minimum_required(VERSION 2.8.7)
project(com.webos.service.camera2 CXX)
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
include_directories(${CMAKE_SOURCE_DIR}/files/)
include_directories(${CMAKE_SOURCE_DIR}/files/sysbus/)
include_directories(${CMAKE_SOURCE_DIR}/files/systemd/)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

#hal
include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/v4l2/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/alsa/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal/utils/)

#service
include_directories(${CMAKE_SOURCE_DIR}/src/service/)

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

#hal2
include_directories(${CMAKE_SOURCE_DIR}/src/hal2/)
include_directories(${CMAKE_SOURCE_DIR}/src/hal2/v4l2)

install(FILES ${CMAKE_SOURCE_DIR}/src/include/camera_hal_if.h DESTINATION
/usr/include)
install(FILES ${CMAKE_SOURCE_DIR}/src/include/camera_hal_types.h DESTINATION
/usr/include)

#libcamera_hal.so
set (SRC_HAL2
     ${CMAKE_SOURCE_DIR}/src/hal2/camera_base_wrapper.cpp
     ${CMAKE_SOURCE_DIR}/src/hal2/camera_hal_if.cpp)
add_library(camera_hal SHARED ${SRC_HAL2})

#v4l2-camera-plugin library
set (SRC_V4L2 ${CMAKE_SOURCE_DIR}/src/hal2/v4l2/v4l2_camera_plugin.cpp)
add_library(v4l2-camera-plugin SHARED ${SRC_V4L2})

#link libraries camera_hal and v4l2-camera-plugin
target_link_libraries(camera_hal v4l2-camera-plugin ${CMAKE_DL_LIBS} pthread)
install(TARGETS v4l2-camera-plugin DESTINATION /usr/lib)
install(TARGETS camera_hal DESTINATION /usr/lib)

#haltestapp project exe
set (SRC_CAMHAL_TEST ${CMAKE_SOURCE_DIR}/src/test/hal2/camera_hal_tests.cpp)
add_executable (hal_test_app ${SRC_CAMHAL_TEST})
target_link_libraries (hal_test_app camera_hal)
install(TARGETS hal_test_app DESTINATION ${WEBOS_INSTALL_SBINDIR})

#libcamera.so
include_directories(${CMAKE_SOURCE_DIR}/src/sdk)
include_directories(${CMAKE_SOURCE_DIR}/src/sdk/notifier)
include_directories(${CMAKE_SOURCE_DIR}/src/sdk/notifier/udev)

set (SRC_SDK ${CMAKE_SOURCE_DIR}/src/sdk/camera.cpp
             ${CMAKE_SOURCE_DIR}/src/sdk/notifier/notifier.cpp
             ${CMAKE_SOURCE_DIR}/src/sdk/notifier/udev/udev_client.cpp)

add_library(camera SHARED ${SRC_SDK})
target_link_libraries (camera camera_hal ${UDEV_LDFLAGS} pthread)
install(TARGETS camera DESTINATION /usr/lib)

#sdktestapp project exe
set (SRC_CAMSDK_TEST ${CMAKE_SOURCE_DIR}/src/test/sdk/sdktestapp.cpp)
add_executable (sdk_test_app ${SRC_CAMSDK_TEST})
target_link_libraries (sdk_test_app camera)
install(TARGETS sdk_test_app DESTINATION ${WEBOS_INSTALL_SBINDIR})

set(HAL_GTEST TRUE CACHE BOOL "Set to TRUE to enable gtest compilation for HAL")
if (HAL_GTEST)
    webos_use_gtest()
    include_directories(${WEBOS_GTEST_INCLUDES})
    add_subdirectory(src/gtest)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

webos_build_system_bus_files()
