# Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License (MPL), v. 2.0.
# If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

include(FetchContent)

find_package(Boost 1.74 REQUIRED)
find_package(fmt REQUIRED)
find_package(Git QUIET)

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

add_library(dlt-mcp-server MODULE src/dlt-mcp-server.cpp src/dashboard.cpp src/settings-dialog.cpp)

target_compile_definitions(dlt-mcp-server PRIVATE DLT_VIEWER_PLUGIN_VERSION="${PLUGIN_VERSION}")

target_link_libraries(dlt-mcp-server ${QT_PREFIX}::Widgets Boost::headers mcp::mcp fmt::fmt)

add_plugin(dlt-mcp-server)

find_program(CLANG_FORMAT NAMES clang-format clang-format-14)
if(CLANG_FORMAT)
    file(GLOB FORMAT_SOURCES src/*.cpp src/*.h)
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT} -i -style=file ${FORMAT_SOURCES}
        COMMENT "Formatting with clang-format"
        VERBATIM)
    add_custom_target(clang-format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror -style=file ${FORMAT_SOURCES}
        COMMENT "Checking formatting"
        VERBATIM)
endif()
