#include <unistd.h>
#include <pty.h>
#include <sys/fcntl.h>
#include "pty.hpp"

namespace terminal
{
    bool Pty::adopt(int masterFd)
    {
        if (masterFd < 0)
            return false;

        close();
        m_masterFd.reset(masterFd);

        return true;
    }

    void Pty::close()
    {
        m_masterFd.reset();
    }

    bool Pty::valid() const
    {
        return m_masterFd.valid();
    }

    int Pty::fd() const
    {
        return m_masterFd.get();
    }

    ssize_t Pty::read(std::span<std::byte> buffer)
    {
        return ::read(fd(), buffer.data(), buffer.size());
    }

    ssize_t Pty::write(std::span<const std::byte> buffer)
    {
        return ::write(fd(), buffer.data(), buffer.size());
    }

    ssize_t Pty::write(std::string_view text)
    {
        return ::write(fd(), text.data(), text.size());
    }

    bool Pty::setNonBlocking(bool enabled)
    {
        int flags = ::fcntl(fd(), F_GETFL);
        if (flags < 0)
            return false;
        if (enabled)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;

        if (::fcntl(fd(), F_SETFL, flags) < 0)
            return false;

        m_nonBlocking = enabled;

        return true;
    }

    bool Pty::isNonBlocking() const
    {
        return m_nonBlocking;
    }

    bool Pty::resize(const TerminalSize &size)
    {
        struct winsize ws{
            .ws_row = size.rows,
            .ws_col = size.cols,
            .ws_xpixel = size.pixelWidth,
            .ws_ypixel = size.pixelHeight,
        };

        return ::ioctl(fd(), TIOCSWINSZ, &ws) == 0;
    }

    TerminalSize Pty::size() const
    {
        struct winsize ws{};
        TerminalSize result;

        if (::ioctl(fd(), TIOCGWINSZ, &ws) == 0)
        {
            result = {
                .rows = ws.ws_row,
                .cols = ws.ws_col,
                .pixelWidth = ws.ws_xpixel,
                .pixelHeight = ws.ws_ypixel,
            };
        }

        return result;
    }

    bool Pty::flush()
    {
        return ::tcflush(fd(), TCIOFLUSH) == 0;
    }
}