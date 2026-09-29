cmake_minimum_required(VERSION 3.22.1)

project("audioforensic")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../native")

include_directories(
    "${NATIVE_DIR}"
)

add_library(
    audioforensic
    SHARED
    "${NATIVE_DIR}/spectral_engine.cpp"
    "${NATIVE_DIR}/audio_decoder.cpp"
    "${NATIVE_DIR}/jni_bridge.cpp"
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
