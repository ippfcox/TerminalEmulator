#include <array>
#include <iostream>

#include "pty/ptyProcess.hpp"

int main()
{
    terminal::PtyProcess proc;

    if (!proc.startShell())
    {
        return 1;
    }

    proc.write("echo hello\n");

    std::array<std::byte, 4096> buffer;

    while (true)
    {
        ssize_t n =
            proc.read(buffer);

        if (n > 0)
        {
            std::cout.write(
                reinterpret_cast<char *>(buffer.data()),
                n);

            std::cout.flush();
        }
    }

    return 0;
}