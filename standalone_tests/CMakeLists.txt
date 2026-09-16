cmake_minimum_required(VERSION 3.10)
project(test_samp_parsers CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
    test_parsers.cpp
    ../app/src/main/cpp/archive/img_archive.cpp
    ../app/src/main/cpp/format/dff_loader.cpp
    ../app/src/main/cpp/format/txd_loader.cpp
    ../app/src/main/cpp/format/ide_parser.cpp
    ../app/src/main/cpp/format/ipl_parser.cpp
    ../app/src/main/cpp/editor/exporter.cpp
    ../app/src/main/cpp/editor/raycast.cpp
    ../app/src/main/cpp/editor/gizmo.cpp
    ../app/src/main/cpp/renderer/camera.cpp
    ../app/src/main/cpp/editor/scene.cpp
)

add_executable(test_samp_parsers ${SOURCE_FILES})
target_include_directories(test_samp_parsers PRIVATE ../app/src/main/cpp)
