find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(speed_client_sdk STATIC
    legacy/legacy_protocol.cpp
    legacy/legacy_client.cpp
    rest/rest_client.cpp
    config/client_settings.cpp
)

target_include_directories(speed_client_sdk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(speed_client_sdk
    PUBLIC
        Qt6::Core
        Qt6::Network
)
