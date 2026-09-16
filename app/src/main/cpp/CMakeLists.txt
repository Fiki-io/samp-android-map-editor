cmake_minimum_required(VERSION 3.22.1)
project("samp_map_editor")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    samp_map_editor
    SHARED
    jni_bridge.cpp
    archive/img_archive.cpp
    format/dff_loader.cpp
    format/txd_loader.cpp
    format/ide_parser.cpp
    format/ipl_parser.cpp
    renderer/camera.cpp
    renderer/gl_renderer.cpp
    editor/exporter.cpp
    editor/raycast.cpp
    editor/gizmo.cpp
    editor/scene.cpp
)

target_include_directories(
    samp_map_editor
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(log-lib log)
find_library(android-lib android)
find_library(gles3-lib GLESv3)
find_library(egl-lib EGL)

target_link_libraries(
    samp_map_editor
    ${log-lib}
    ${android-lib}
    ${gles3-lib}
    ${egl-lib}
)
