cmake_minimum_required(VERSION 3.31)

project(TerminalEmulatorSrc)
set(TARGET TerminalEmulator)

list(APPEND SRCS
    main.cpp
)

add_executable(${TARGET} ${SRCS})


target_include_directories(${TARGET}
    PRIVATE
)

target_link_directories(${TARGET}
    PRIVATE
)

target_link_libraries(${TARGET}
    PRIVATE
    spdlog::spdlog
    lvgl::lvgl
    libtsm-imported
    util
)


