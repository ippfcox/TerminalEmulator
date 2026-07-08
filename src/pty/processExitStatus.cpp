#include <sys/wait.h>
#include "processExitStatus.hpp"

namespace terminal
{
    ProcessExitStatus ProcessExitStatus::fromWaitStatus(int status)
    {
        ProcessExitStatus result;

        if (WIFEXITED(status))
            result.m_exitCode = WEXITSTATUS(status);
        if (WIFSIGNALED(status))
            result.m_signal = WTERMSIG(status);
#ifdef WCOREDUMP
        result.m_coreDumped = WCOREDUMP(status);
#endif

        return result;
    }

    bool ProcessExitStatus::exitedNormally() const
    {
        return m_exitCode.has_value();
    }

    bool ProcessExitStatus::signaled() const
    {
        return m_signal.has_value();
    }

    bool ProcessExitStatus::coreDumped() const
    {
        return m_coreDumped;
    }

    int ProcessExitStatus::exitCode() const
    {
        return m_exitCode.value_or(-1);
    }

    int ProcessExitStatus::signal() const
    {
        return m_signal.value_or(-1);
    }

}