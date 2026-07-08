#pragma once

#include <span>
#include <string>
#include <string_view>

#include "terminalSize.hpp"
#include "uniqueFd.hpp"

namespace terminal
{
    class Pty
    {
    public:
        Pty() = default;
        ~Pty() = default;
        Pty(const Pty &) = delete;
        Pty &operator=(const Pty &) = delete;
        Pty(Pty &&) noexcept = default;
        Pty &operator=(Pty &&) = default;

    public:
        bool adopt(int masterFd);
        void close();

    public:
        [[nodiscard]]
        bool valid() const;
        [[nodiscard]]
        int fd() const;

    public:
        ssize_t read(std::span<std::byte> buffer);
        ssize_t write(std::span<const std::byte> buffer);
        ssize_t write(std::string_view text);

    public:
        bool setNonBlocking(bool enabled);
        [[nodiscard]]
        bool isNonBlocking() const;

    public:
        bool resize(const TerminalSize &size);
        [[nodiscard]]
        TerminalSize size() const;

    public:
        bool flush();

    private:
        UniqueFd m_masterFd;
        bool m_nonBlocking{false};
    };
}