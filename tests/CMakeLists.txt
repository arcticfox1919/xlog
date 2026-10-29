# Copyright (C) 2026. Licensed under the MIT License.
cmake_minimum_required(VERSION 3.22)
project(xlog_file_policy_tests LANGUAGES C CXX)

if(ANDROID)
    add_subdirectory(../mars/libraries/mars_xlog_sdk native)
else()
    set(MARS_BUILD_XLOG_ONLY ON CACHE BOOL "Build only xlog dependencies" FORCE)
    add_subdirectory(../mars native)
endif()

add_executable(xlog_file_policy_test file_policy_test.cc)
target_compile_features(xlog_file_policy_test PRIVATE cxx_std_14)
set_target_properties(xlog_file_policy_test PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES)
target_include_directories(xlog_file_policy_test PRIVATE .. ../mars ../mars/comm ../mars/comm/xlogger)
target_link_libraries(xlog_file_policy_test PRIVATE xlog mars-boost comm libzstd_static)
if(ANDROID)
    target_link_libraries(xlog_file_policy_test PRIVATE log z)
elseif(APPLE)
    target_link_libraries(xlog_file_policy_test PRIVATE z "-framework Foundation" "-framework SystemConfiguration")
elseif(UNIX)
    target_link_libraries(xlog_file_policy_test PRIVATE z pthread)
endif()

enable_testing()
add_test(NAME file_policy COMMAND xlog_file_policy_test)
# Uses the production delayed cleanup rather than changing its schedule for tests.
add_test(NAME file_policy_expiry COMMAND xlog_file_policy_test --expiry)
set_tests_properties(file_policy_expiry PROPERTIES TIMEOUT 150)
