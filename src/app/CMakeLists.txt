find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 QuickLayouts QuickDialogs2 Svg QuickEffects
)

if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

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

qt_add_executable(SpeedClient ${SPEEDCLIENT_APP_SOURCES})

# 嵌入 QML 模块，供 import scanner 打包 Android 插件（不可用 qt_target_qml_sources 于纯 executable）
qt_add_qml_module(SpeedClient
    URI "SpeedClient"
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        Main.qml
        pages/ConnectionPage.qml
        pages/RestPage.qml
        pages/LegacyPage.qml
        components/ClientPageLayout.qml
        components/EventLogPanel.qml
)

target_include_directories(SpeedClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk
    ${SENSEDESIGN_ROOT}/src/cpp_tokens
)

target_link_libraries(SpeedClient PRIVATE
    speed_client_sdk
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickLayouts
    Qt6::QuickDialogs2
    Qt6::Svg
    Qt6::QuickEffects
)

if(ANDROID)
    target_link_libraries(SpeedClient PRIVATE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,SenseAppShell>"
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,SenseDesign>")
else()
    target_link_libraries(SpeedClient PRIVATE SenseAppShell SenseDesign)
endif()

add_dependencies(SpeedClient SenseAppShell SenseDesign)

set(_sensedesign_qml_build "${CMAKE_BINARY_DIR}/sensedesign_build")
set_target_properties(SpeedClient PROPERTIES
    QT_QML_IMPORT_PATH "${_sensedesign_qml_build}"
)

if(ANDROID)
    set_target_properties(SpeedClient PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../android"
        QT_ANDROID_LEGACY_PACKAGING FALSE
    )
endif()

if(WIN32)
    set_target_properties(SpeedClient PROPERTIES
        WIN32_EXECUTABLE $<IF:$<CONFIG:Debug>,FALSE,TRUE>)
endif()

target_compile_definitions(SpeedClient PRIVATE
    $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

qt6_import_qml_plugins(SpeedClient)
