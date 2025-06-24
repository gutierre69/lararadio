cmake_minimum_required(VERSION 3.16)

# Versão principal
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 4)

# Caminho do arquivo de build number
set(BUILD_NUMBER_FILE "${CMAKE_CURRENT_BINARY_DIR}/build_number.txt")

# Lê build number do arquivo ou inicia em 0
if(EXISTS ${BUILD_NUMBER_FILE})
    file(READ ${BUILD_NUMBER_FILE} BUILD_NUMBER)
    string(STRIP "${BUILD_NUMBER}" BUILD_NUMBER)
else()
    set(BUILD_NUMBER 0)
endif()

# Incrementa
math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")

# Salva de volta
file(WRITE ${BUILD_NUMBER_FILE} "${BUILD_NUMBER}\n")

# Junta tudo
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(PROJECT_FULL_VERSION "${PROJECT_VERSION}+build.${BUILD_NUMBER}")

# Exporta para uso em código
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

message(STATUS "Build version: ${PROJECT_FULL_VERSION}")

project(appLaraRadio VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Multimedia MultimediaWidgets)
set(QT_USE_QTMULTIMEDIA TRUE)
set(QT_USE_QTMULTIMEDIAWIDGETS TRUE)
set(QT_DEBUG_PLUGINS TRUE)

qt_standard_project_setup(REQUIRES 6.8)

include_directories(/usr/include/taglib)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

qt_add_resources(RESOURCE_FILES resources.qrc)

qt_add_executable(appLaraRadio
    main.cpp
    resources.qrc
    ${PROJECT_SOURCES}
)

qt_add_qml_module(appLaraRadio
    URI LaraRadio
    VERSION ${PROJECT_VERSION}
    QML_FILES

        RESOURCES mainwindow.ui
        SOURCES mainwindow.h mainwindow.cpp
        SOURCES audioplayer.cpp
        SOURCES audioplayer.h
        SOURCES timerclock.h
        SOURCES timerclock.cpp
        SOURCES about_dialog.h about_dialog.cpp
        RESOURCES about_dialog.ui
        RESOURCES resources.qrc
        SOURCES configdialog.h configdialog.cpp
        RESOURCES configdialog.ui
        SOURCES custonIconProvider.h
        SOURCES version.h.in
        RESOURCES
        SOURCES vumeter.h vumeter.cpp
        SOURCES buttonhole.h buttonhole.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appLaraRadio PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appLaraRadio
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_sources(appLaraRadio PRIVATE ${RESOURCE_FILES})

target_link_libraries(appLaraRadio
    PRIVATE
    Qt6::Quick
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    tag
)

include(GNUInstallDirs)
install(TARGETS appLaraRadio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
