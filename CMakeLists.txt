cmake_minimum_required(VERSION 3.26)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
endif()

project(SpeedClient
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Speed.Server example client (REST + Legacy streaming)"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_LINK_DEPENDS_ON_STATIC_LIBRARIES ON)

option(SPEEDCLIENT_BUILD_APP "Build SpeedClient application" ON)

set(SENSE_DESIGN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SenseDesign"
    CACHE PATH "Path to SenseDesign (shadcn QML submodule tree)")

if(NOT EXISTS "${SENSE_DESIGN_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "SenseDesign submodule missing at third_party/SenseDesign.\n"
        "Run: git submodule update --init third_party/SenseDesign\n"
        "Or:  .\\tools\\setup_submodules.ps1\n"
        "Override path: -DSENSE_DESIGN_DIR=<path>")
endif()

set(BUILD_SENSEDESIGN_DEMO OFF CACHE BOOL "Build SenseDesign doc demo" FORCE)
add_subdirectory("${SENSE_DESIGN_DIR}" "${CMAKE_BINARY_DIR}/_sub/SenseDesign")

add_subdirectory(src/sdk)

if(SPEEDCLIENT_BUILD_APP)
    add_subdirectory(src/app)
endif()

message(STATUS "SpeedClient ${PROJECT_VERSION} | SenseDesign: ${SENSE_DESIGN_DIR}")
