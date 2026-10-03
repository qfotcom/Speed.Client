find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 QuickLayouts QuickDialogs2 Svg QuickEffects Graphs
)

if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0002 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

set(SPEEDCLIENT_QML_LINK_TOKEN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/sense_qml_static_link_token.cpp")
set(SPEEDCLIENT_QML_LINK_TOKEN "${CMAKE_BINARY_DIR}/speedclient_qml_static_link_token.cpp")

set(SPEEDCLIENT_APP_SOURCES
    main.cpp
    ClientBackend.cpp
    ClientBackend.h
)

if(ANDROID)
    list(APPEND SPEEDCLIENT_APP_SOURCES
        AndroidStartupLog.cpp
        AndroidStartupLog.h
        AndroidSystemUiBridge.cpp
        AndroidSystemUiBridge.h
    )
endif()

if(WIN32)
    list(APPEND SPEEDCLIENT_APP_SOURCES
        "${SENSE_DESIGN_DIR}/demo/WindowMinimizeBridge.cpp"
        "${SENSE_DESIGN_DIR}/demo/WindowMinimizeBridge.h"
    )
endif()

qt_add_executable(SpeedClient ${SPEEDCLIENT_APP_SOURCES})
set_target_properties(SpeedClient PROPERTIES FOLDER "qtc_runnable")

qt_add_qml_module(SpeedClient
    URI "SpeedClient"
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        Main.qml
        page-config.js
        pages/ConnectionPage.qml
        pages/RestPage.qml
        pages/LegacyPage.qml
        components/ClientMobilePage.qml
        components/ClientStatusBadge.qml
        components/ClientStatusAlert.qml
        components/ClientHttpStatus.js
        components/EventLogPanel.qml
)

if(WIN32)
    target_include_directories(SpeedClient PRIVATE "${SENSE_DESIGN_DIR}/demo")
    target_compile_definitions(SpeedClient PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(SpeedClient PRIVATE dwmapi)
    set_target_properties(SpeedClient PROPERTIES
        WIN32_EXECUTABLE $<IF:$<CONFIG:Debug>,FALSE,TRUE>
        QT_TARGET_WINDOWS_DPI_AWARENESS "PerMonitorV2")
endif()

target_include_directories(SpeedClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk
)

if(MSVC OR ANDROID)
    target_link_libraries(SpeedClient PRIVATE
        speed_client_sdk
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,SenseAppShell>"
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,SenseDesign>"
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickLayouts
        Qt6::QuickDialogs2
        Qt6::Svg
        Qt6::QuickEffects
        Qt6::Graphs
    )
else()
    target_link_libraries(SpeedClient PRIVATE
        speed_client_sdk
        SenseAppShell
        SenseDesign
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickLayouts
        Qt6::QuickDialogs2
        Qt6::Svg
        Qt6::QuickEffects
        Qt6::Graphs
    )
endif()

add_dependencies(SpeedClient SenseAppShell SenseDesign)

add_custom_command(
    OUTPUT "${SPEEDCLIENT_QML_LINK_TOKEN}"
    COMMAND ${CMAKE_COMMAND} -E copy "${SPEEDCLIENT_QML_LINK_TOKEN_SRC}" "${SPEEDCLIENT_QML_LINK_TOKEN}"
    DEPENDS
        "${SPEEDCLIENT_QML_LINK_TOKEN_SRC}"
        SenseDesign
        SenseAppShell
    COMMENT "Refresh SpeedClient link token (static QML libraries changed)"
)
target_sources(SpeedClient PRIVATE "${SPEEDCLIENT_QML_LINK_TOKEN}")

set(_sensedesign_qml_build "${CMAKE_BINARY_DIR}/_sub/SenseDesign")
set_target_properties(SpeedClient PROPERTIES
    QT_QML_IMPORT_PATH "${_sensedesign_qml_build}"
)

if(ANDROID)
    set_target_properties(SpeedClient PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../android"
        QT_ANDROID_LEGACY_PACKAGING FALSE
    )
endif()

target_compile_definitions(SpeedClient PRIVATE
    $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

if(NOT ANDROID AND NOT IOS AND NOT MSVC)
    qt6_import_qml_plugins(SpeedClient)
endif()
