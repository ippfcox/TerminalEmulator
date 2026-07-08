cmake_minimum_required(VERSION 3.31)

project(TerminalEmulatorSrc)
set(TARGET TerminalEmulator)

list(APPEND SRCS
    # pty
    pty/processExitStatus.cpp
    pty/pty.cpp
    pty/ptyProcess.cpp
    pty/terminalSize.hpp
    pty/uniqueFd.cpp

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
    util
)


