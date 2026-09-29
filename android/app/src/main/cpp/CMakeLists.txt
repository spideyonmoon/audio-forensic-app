cmake_minimum_required(VERSION 3.22.1)

project("audioforensic")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(
    audioforensic
    SHARED
    spectral_engine.cpp
    audio_decoder.cpp
    jni_bridge.cpp
)

find_library(
    log-lib
    log
)

find_library(
    android-lib
    android
)

target_link_libraries(
    audioforensic
    ${log-lib}
    ${android-lib}
)
