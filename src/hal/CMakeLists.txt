# @@@LICENSE
#
# Copyright (c) 2013 LG Electronics, Inc.
#
# LICENSE@@@

cmake_minimum_required(VERSION 2.8.7)
project(HAL C CXX)
include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC4)

# ---
# find required packages
include(FindPkgConfig)

pkg_check_modules(JSONC REQUIRED json-c)
webos_add_compiler_flags(ALL ${JSONC_CFLAGS})
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})
pkg_check_modules(LUNASERVICE REQUIRED luna-service2)
webos_add_compiler_flags(ALL ${LUNASERVICE_CFLAGS})
pkg_check_modules(ALSA REQUIRED alsa)
webos_add_compiler_flags(ALL ${ALSA_CFLAGS})
pkg_check_modules(EXTDEVCTRL REQUIRED extdevctrl)
webos_add_compiler_flags(ALL ${EXTDEVCTRL_CFLAGS})
pkg_check_modules(PMLOG REQUIRED PmLogLib)
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})
pkg_check_modules(PNG REQUIRED libpng12)
webos_add_compiler_flags(ALL ${PNG_CFLAGS})

webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)


# ---
# add include files
webos_add_compiler_flags(ALL -I${CMAKE_SOURCE_DIR}/include/public -std=c++11)

include_directories(/home/gururaj.patil/update/com.webos.service.camera/src/)
include_directories(/home/gururaj.patil/update/com.webos.service.camera/src/include)
include_directories(/home/gururaj.patil/update/com.webos.service.camera/src/service/)
include_directories(/home/gururaj.patil/update/com.webos.service.camera/tests/)
include_directories(${CMAKE_SOURCE_DIR}/v4l2/)
include_directories(${CMAKE_SOURCE_DIR}/alsa/)
include_directories(${CMAKE_SOURCE_DIR}/utils/)
include_directories(${PMLOG_INCLUDE_DIRS})


#Library
set(mylibSRCS
  ${CMAKE_SOURCE_DIR}/v4l2/hal_v4l2.cpp
  )

add_library (v4l2 SHARED ${mylibSRCS})
# ---
# create executable file
set(BIN_NAME HAL)

set(SRC_LIST
    # main
    /home/gururaj.patil/update/com.webos.service.camera/tests/hal_test.cpp
    ${CMAKE_SOURCE_DIR}/utils/camshm.cpp
    ${CMAKE_SOURCE_DIR}/v4l2/hal_v4l2.cpp
    ${CMAKE_SOURCE_DIR}/alsa/hal_alsa.cpp
    # interface
    ${CMAKE_SOURCE_DIR}/hal.cpp
    )

# ---
# set libs list
set(LIB_LIST
    pthread
    udev
)


add_executable(${BIN_NAME} ${SRC_LIST})
set_target_properties(${BIN_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries (${BIN_NAME}
            ${LIB_LIST}
            ${JSONC_LDFLAGS}
            ${GLIB2_LDFLAGS}
            ${LUNASERVICE_LDFLAGS}
            ${ALSA_LDFLAGS}
            ${EXTDEVCTRL_LDFLAGS}
            ${PMLOG_LDFLAGS}
            ${PNG_LDFLAGS}
            )


# ---
# install hal to /usr/sbin, upstart script to /etc/event.d
# install to /usr/sbin
webos_build_program(ADMIN)
