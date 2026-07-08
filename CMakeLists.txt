cmake_minimum_required(VERSION 3.31)

project(TerminalEmulator VERSION 0.0)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
# add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/lvgl)
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/spdlog)