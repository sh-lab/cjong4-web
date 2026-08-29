cmake_minimum_required(VERSION 3.16)
project(cjong4_web VERSION 0.1.0 LANGUAGES C)

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
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CJ4_WEB_SITE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/web" "${CJ4_WEB_SITE_DIR}"
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
