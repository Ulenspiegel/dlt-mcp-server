# Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License (MPL), v. 2.0.
# If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

include(FetchContent)

find_package(Boost 1.74 REQUIRED)
find_package(Git QUIET)

set(FMT_HEADER_ONLY ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.2.0
)
FetchContent_MakeAvailable(fmt)
add_library(fmt::fmt ALIAS fmt)

# Derive version from git tags; fall back to 0.0.0 if no tags or git unavailable.
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --match "v*"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE PLUGIN_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_DESCRIBE_RESULT
    )
    if(NOT GIT_DESCRIBE_RESULT EQUAL 0)
        set(PLUGIN_VERSION "0.0.0")
    else()
        string(REGEX REPLACE "^v" "" PLUGIN_VERSION "${PLUGIN_VERSION}")
    endif()
else()
    set(PLUGIN_VERSION "0.0.0")
endif()

FetchContent_Declare(
        cpp-mcp
        GIT_REPOSITORY https://github.com/hkr04/cpp-mcp.git
        GIT_TAG main
)
FetchContent_MakeAvailable(cpp-mcp)
add_library(mcp::mcp ALIAS mcp)
target_include_directories(mcp INTERFACE ${cpp-mcp_SOURCE_DIR}/common ${cpp-mcp_SOURCE_DIR}/include)

FetchContent_Declare(
        md4c
        GIT_REPOSITORY https://github.com/mity/md4c.git
        GIT_TAG release-0.5.3
)
set(BUILD_MD2HTML_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_MakeAvailable(md4c)
add_library(md4c::md4c-html ALIAS md4c-html)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HARMFUL OFF CACHE BOOL "" FORCE)
set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(spdlog)

add_library(dlt-mcp-server MODULE src/dlt-mcp-server.cpp src/dashboard.cpp src/report-cache.cpp src/settings-dialog.cpp)

target_compile_definitions(dlt-mcp-server PRIVATE DLT_VIEWER_PLUGIN_VERSION="${PLUGIN_VERSION}")

target_link_libraries(dlt-mcp-server ${QT_PREFIX}::Widgets Boost::headers mcp::mcp fmt::fmt md4c::md4c-html spdlog::spdlog_header_only)

add_plugin(dlt-mcp-server)

find_program(CLANG_FORMAT NAMES clang-format clang-format-14)
if(CLANG_FORMAT)
    file(GLOB FORMAT_SOURCES src/*.cpp src/*.h)
    add_custom_target(mcp-format
        COMMAND ${CLANG_FORMAT} -i -style=file ${FORMAT_SOURCES}
        COMMENT "Formatting with clang-format"
        VERBATIM)
    add_custom_target(mcp-format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror -style=file ${FORMAT_SOURCES}
        COMMENT "Checking formatting"
        VERBATIM)
endif()
