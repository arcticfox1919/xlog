cmake_minimum_required(VERSION 3.22.1)

project(mars_xlog_sdk)

set(MARS_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Installation directory" FORCE)

include_directories(${MARS_ROOT})
include_directories(${MARS_ROOT}/..)
include_directories(${MARS_ROOT}/openssl/include)

add_subdirectory(${MARS_ROOT}/comm comm)
add_subdirectory(${MARS_ROOT}/boost boost)
add_subdirectory(${MARS_ROOT}/xlog xlog)

option(ZSTD_BUILD_STATIC "BUILD STATIC LIBRARIES" ON)
option(ZSTD_BUILD_SHARED "BUILD SHARED LIBRARIES" OFF)
set(ZSTD_SOURCE_DIR "${MARS_ROOT}/zstd")
set(LIBRARY_DIR ${ZSTD_SOURCE_DIR}/lib)
include(GNUInstallDirs)
add_subdirectory(${MARS_ROOT}/zstd/build/cmake/lib zstd)

foreach(MARS_XLOG_TARGET comm mars-boost xlog libzstd_static)
    if(TARGET ${MARS_XLOG_TARGET})
        target_compile_options(${MARS_XLOG_TARGET}
                PRIVATE
                -Wno-error=deprecated-builtins
                -Wno-error=deprecated-declarations)
    endif()
endforeach()

find_library(log-lib log)
find_library(z-lib z)

add_library(marsxlog SHARED
        ${MARS_ROOT}/libraries/mars_android_sdk/jni/JNI_OnLoad.cc
        ${CMAKE_CURRENT_LIST_DIR}/jni/import.cc)

get_filename_component(EXPORT_XLOG_EXP_FILE
        ${MARS_ROOT}/libraries/mars_android_sdk/jni/export.exp
        ABSOLUTE)

set(SELF_XLOG_LINKER_FLAG "-Wl,--gc-sections -Wl,--undefined-version -Wl,--version-script=${EXPORT_XLOG_EXP_FILE}")
if(ANDROID_ABI STREQUAL "x86")
    set(SELF_XLOG_LINKER_FLAG "${SELF_XLOG_LINKER_FLAG} -Wl,--no-warn-shared-textrel")
endif()

target_link_options(marsxlog
        PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384")

target_link_libraries(marsxlog
        PRIVATE
        ${SELF_XLOG_LINKER_FLAG}
        xlog
        mars-boost
        comm
        libzstd_static
        ${log-lib}
        ${z-lib})
