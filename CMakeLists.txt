# Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License (MPL), v. 2.0.
# If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Boost 1.74 REQUIRED)
find_package(Git QUIET)
find_package(GTest QUIET)

set(FMT_HEADER_ONLY ON CACHE BOOL "" FORCE)
add_subdirectory(external/fmt)
add_library(fmt::fmt ALIAS fmt)

# Derive version from git tags; fall back to 0.0.0 if no tags or git unavailable.
if (GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --always --match "v*"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE PLUGIN_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE GIT_DESCRIBE_RESULT
    )
    if (NOT GIT_DESCRIBE_RESULT EQUAL 0)
        set(PLUGIN_VERSION "0.0.0")
    else ()
        string(REGEX REPLACE "^v" "" PLUGIN_VERSION "${PLUGIN_VERSION}")
    endif ()
else ()
    set(PLUGIN_VERSION "0.0.0")
endif ()

# Pinned to a specific commit instead of a moving branch (main) for supply-chain
# safety. The transport/network code lives in this dependency, so its contents
# must be reproducible. Update deliberately after reviewing upstream changes.
set(cpp-mcp_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-mcp)
add_subdirectory(${cpp-mcp_SOURCE_DIR})
add_library(mcp::mcp ALIAS mcp)
target_include_directories(mcp INTERFACE ${cpp-mcp_SOURCE_DIR}/common ${cpp-mcp_SOURCE_DIR}/include)

set(BUILD_MD2HTML_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(external/md4c)
add_library(md4c::md4c-html ALIAS md4c-html)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HARMFUL OFF CACHE BOOL "" FORCE)
set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)
add_subdirectory(external/spdlog)

if (GTest_FOUND)
    enable_testing()
    add_executable(test_bloom_filter test/test_bloom_filter.cpp src/bloom_filter.cpp)
    target_include_directories(test_bloom_filter PRIVATE src)
    target_link_libraries(test_bloom_filter PRIVATE GTest::gtest_main ${QT_PREFIX}::Core)
    add_test(NAME mcp-test_bloom_filter COMMAND $<TARGET_FILE:test_bloom_filter>)

    add_custom_target(mcp-test
            COMMAND ${CMAKE_CTEST_COMMAND} -R mcp-test_
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running all mcp plugin tests"
            VERBATIM)
    add_dependencies(mcp-test test_bloom_filter)
endif ()

add_library(
    dlt-mcp-server MODULE
    src/basic_index.cpp
    src/bloom_filter.cpp
    src/dlt_mcp_server.cpp
    src/ngram_index.cpp
    src/report_cache.cpp
    src/report_storage.cpp
    src/ui/dashboard.cpp
    src/ui/report_browser.cpp
    src/ui/report_header.cpp
    src/ui/report_table_model.cpp
    src/ui/settings_dialog.cpp
)

target_include_directories(dlt-mcp-server PRIVATE src)

target_compile_definitions(dlt-mcp-server PRIVATE DLT_VIEWER_PLUGIN_VERSION="${PLUGIN_VERSION}")

target_link_libraries(dlt-mcp-server ${QT_PREFIX}::Widgets Boost::headers mcp::mcp fmt::fmt md4c::md4c-html spdlog::spdlog_header_only)

add_plugin(dlt-mcp-server)

find_program(CLANG_FORMAT NAMES clang-format clang-format-14)
if (CLANG_FORMAT)
    file(GLOB_RECURSE FORMAT_SOURCES src/*.cpp src/*.h)
    add_custom_target(mcp-format
            COMMAND ${CLANG_FORMAT} -i -style=file ${FORMAT_SOURCES}
            COMMENT "Formatting with clang-format"
            VERBATIM)
    add_custom_target(mcp-format-check
            COMMAND ${CLANG_FORMAT} --dry-run --Werror -style=file ${FORMAT_SOURCES}
            COMMENT "Checking formatting"
            VERBATIM)
endif ()
