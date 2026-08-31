cmake_minimum_required(VERSION 3.31)

project(TerminalEmulator VERSION 0.0)

set(CMAKE_CXX_STANDARD 20)

find_program(MESON_EXECUTABLE meson REQUIRED)
set(LIBTSM_SRC_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libtsm)
set(LIBTSM_BUILD_DIR ${CMAKE_BINARY_DIR}/libtsm-build/meson-build)

add_library(libtsm-imported SHARED IMPORTED)
set_target_properties(libtsm-imported
    PROPERTIES
    IMPORTED_LOCATION ${LIBTSM_BUILD_DIR}/src/tsm/libtsm.so
    INTERFACE_INCLUDE_DIRECTORIES ${LIBTSM_SRC_DIR}/src/tsm;${LIBTSM_SRC_DIR}/src/shared;${LIBTSM_SRC_DIR}/external)

add_subdirectory(src)
# add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/lvgl)
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/spdlog)

include(ExternalProject)

ExternalProject_Add(libtsm
    PREFIX ${CMAKE_BINARY_DIR}/libtsm-build
    SOURCE_DIR ${LIBTSM_SRC_DIR}
    CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup --buildtype=release ${LIBTSM_SRC_DIR} ${LIBTSM_BUILD_DIR} -Dtests=false -Ddefault_library=static
    BUILD_COMMAND ${MESON_EXECUTABLE} compile -C ${LIBTSM_BUILD_DIR}
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE)
add_dependencies(TerminalEmulator libtsm)
