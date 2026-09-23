cmake_minimum_required(VERSION 3.26)

project(SpeedClient
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Speed.Server example client (REST + Legacy streaming)"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(SPEEDCLIENT_BUILD_APP "Build SpeedClient application" ON)

set(SENSEDESIGN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../SenseDesign"
    CACHE PATH "Path to SenseDesign repository")

if(NOT EXISTS "${SENSEDESIGN_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "SenseDesign not found at ${SENSEDESIGN_ROOT}. "
        "Clone sibling repo or set -DSENSEDESIGN_ROOT=...")
endif()

set(BUILD_SENSEDESIGN_DEMO OFF CACHE BOOL "" FORCE)
add_subdirectory("${SENSEDESIGN_ROOT}" sensedesign_build)

add_subdirectory(src/sdk)

if(SPEEDCLIENT_BUILD_APP)
    add_subdirectory(src/app)
endif()

message(STATUS "SpeedClient ${PROJECT_VERSION} | SenseDesign: ${SENSEDESIGN_ROOT}")
