cmake_minimum_required(VERSION 3.16)
project(cjong4_web VERSION 0.9.4 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(CJ4_WEB_BUILD_TESTS "Build cjong4-web native tests" ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/cjong4-opponent/CMakeLists.txt")
    message(FATAL_ERROR
        "cjong4-opponent is missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/cjong4-mjai/CMakeLists.txt")
    message(FATAL_ERROR
        "cjong4-mjai is missing. Run: git submodule update --init --recursive")
endif()

set(CJ4_OPPONENT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CJ4_OPPONENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/cjong4-opponent EXCLUDE_FROM_ALL)
set(CJ4_MJAI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/cjong4-mjai EXCLUDE_FROM_ALL)

set(CJ4_WEB_SOURCES
    src/game_session.c
    src/web_api.c
)

if(EMSCRIPTEN)
    add_executable(cjong4_web ${CJ4_WEB_SOURCES})
else()
    add_library(cjong4_web STATIC ${CJ4_WEB_SOURCES})
    add_library(cjong4::web ALIAS cjong4_web)
endif()

target_include_directories(cjong4_web PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(cjong4_web PUBLIC cjong4::opponents cjong4::mjai)

if(MSVC)
    target_compile_options(cjong4_web PRIVATE /W4)
else()
    target_compile_options(cjong4_web PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
    set(CJ4_WEB_SITE_DIR "${CMAKE_CURRENT_BINARY_DIR}/site")
    set(CJ4_WEB_ASSET_VERSION "" CACHE STRING
        "Browser asset cache version (defaults to the Git commit)")

    if(NOT CJ4_WEB_ASSET_VERSION)
        find_package(Git QUIET)
        if(Git_FOUND)
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE CJ4_WEB_GIT_RESULT
                OUTPUT_VARIABLE CJ4_WEB_ASSET_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(CJ4_WEB_GIT_RESULT EQUAL 0)
                execute_process(
                    COMMAND "${GIT_EXECUTABLE}" status --porcelain
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    OUTPUT_VARIABLE CJ4_WEB_GIT_STATUS
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
                if(CJ4_WEB_GIT_STATUS)
                    string(TIMESTAMP CJ4_WEB_DIRTY_TIMESTAMP "%Y%m%d%H%M%S" UTC)
                    string(APPEND CJ4_WEB_ASSET_VERSION "-dirty-${CJ4_WEB_DIRTY_TIMESTAMP}")
                endif()
            endif()
        endif()
        if(NOT CJ4_WEB_ASSET_VERSION)
            set(CJ4_WEB_ASSET_VERSION "dev")
        endif()
    endif()
    string(REGEX REPLACE "[^A-Za-z0-9._-]" "-"
        CJ4_WEB_ASSET_VERSION "${CJ4_WEB_ASSET_VERSION}")

    file(MAKE_DIRECTORY "${CJ4_WEB_SITE_DIR}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/web/index.html"
        "${CJ4_WEB_SITE_DIR}/index.html"
        @ONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/web/app.js"
        "${CJ4_WEB_SITE_DIR}/app.js"
        @ONLY
    )
    message(STATUS "Browser asset version: ${CJ4_WEB_ASSET_VERSION}")

    set_target_properties(cjong4_web PROPERTIES
        OUTPUT_NAME "cjong4-web"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CJ4_WEB_SITE_DIR}"
    )
    target_link_options(cjong4_web PRIVATE
        "--no-entry"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sEXPORT_NAME=createCjong4WebModule"
        "-sENVIRONMENT=web"
        "-sFILESYSTEM=0"
        "-sEXPORTED_RUNTIME_METHODS=UTF8ToString"
    )
    add_custom_target(cjong4_web_site ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CJ4_WEB_SITE_DIR}/licenses"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/web/styles.css"
            "${CJ4_WEB_SITE_DIR}/styles.css"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            "${CJ4_WEB_SITE_DIR}/LICENSE.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
            "${CJ4_WEB_SITE_DIR}/THIRD_PARTY_NOTICES.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/licenses" "${CJ4_WEB_SITE_DIR}/licenses"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CJ4_WEB_SITE_DIR}/assets"
        DEPENDS cjong4_web
        VERBATIM
    )
endif()

if(CJ4_WEB_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    add_executable(cjong4_web_tests tests/test_web_api.c)
    target_link_libraries(cjong4_web_tests PRIVATE cjong4::web)
    add_test(NAME cjong4_web_tests COMMAND cjong4_web_tests)
endif()
