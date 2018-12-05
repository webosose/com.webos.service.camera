# @@@LICENSE
#
# Copyright (c) 2013 LG Electronics, Inc.
#
# LICENSE@@@

project(com.webos.service.camera3 CXX)

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

webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

#service2
include_directories(${CMAKE_SOURCE_DIR}/src/service2/)
include_directories(${CMAKE_SOURCE_DIR}/src/service2/notifier/)
include_directories(${CMAKE_SOURCE_DIR}/src/service2/notifier/pdm/)
set(SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/service2/camera_service.cpp
    ${CMAKE_SOURCE_DIR}/src/service2/json_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/service2/service_types.cpp
    ${CMAKE_SOURCE_DIR}/src/service2/notifier/notifier.cpp
    ${CMAKE_SOURCE_DIR}/src/service2/notifier/pdm/pdm_client.cpp
    )

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})
