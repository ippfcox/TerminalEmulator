#pragma once

namespace terminal
{
    class UniqueFd
    {
    public:
        UniqueFd() = default;
        explicit UniqueFd(int fd);
        ~UniqueFd();
        UniqueFd(const UniqueFd &) = delete;
        UniqueFd &operator=(const UniqueFd &) = delete;
        UniqueFd(UniqueFd &&other) noexcept;
        UniqueFd &operator=(UniqueFd &&other) noexcept;

    public:
        void reset(int newFd = -1);

        [[nodiscard]]
        int release();
        [[nodiscard]]
        bool valid() const;
        [[nodiscard]]
        int get() const;

        explicit operator bool() const;

    private:
        int m_fd{-1};
    };
}