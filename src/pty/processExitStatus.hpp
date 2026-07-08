#pragma once

#include <optional>

namespace terminal
{
    class ProcessExitStatus
    {
    public:
        ProcessExitStatus() = default;
        static ProcessExitStatus fromWaitStatus(int status);

    public:
        bool exitedNormally() const;
        bool signaled() const;
        bool coreDumped() const;
        int exitCode() const;
        int signal() const;

    private:
        std::optional<int> m_exitCode;
        std::optional<int> m_signal;
        bool m_coreDumped{false};
    };
}