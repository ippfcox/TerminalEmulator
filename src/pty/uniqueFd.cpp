#include <unistd.h>
#include "uniqueFd.hpp"

namespace terminal
{
    UniqueFd::UniqueFd(int fd)
        : m_fd(fd)
    {
    }

    UniqueFd::~UniqueFd()
    {
        reset();
    }

    UniqueFd::UniqueFd(UniqueFd &&other) noexcept
    {
        m_fd = other.m_fd;
        other.m_fd = -1;
    }

    UniqueFd &UniqueFd::operator=(UniqueFd &&other) noexcept
    {
        if (this == &other)
            return *this;

        reset();

        m_fd = other.m_fd;
        other.m_fd = -1;

        return *this;
    }

    void UniqueFd::reset(int newFd)
    {
        if (m_fd >= 0)
            ::close(m_fd);

        m_fd = newFd;
    }

    int UniqueFd::release()
    {
        int fd = m_fd;
        m_fd = -1;

        return fd;
    }

    bool UniqueFd::valid() const
    {
        return m_fd >= 0;
    }

    int UniqueFd::get() const
    {
        return m_fd;
    }

    UniqueFd::operator bool() const
    {
        return valid();
    }
}