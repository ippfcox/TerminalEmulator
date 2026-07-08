#include <thread>
#include <pty.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ptyProcess.hpp"

namespace terminal
{
    PtyProcess::~PtyProcess()
    {
        if (m_running)
        {
            terminate();
            wait();
        }
    }

    bool PtyProcess::spawn(std::string_view executable, const std::vector<std::string> &args, const SpawnOptions &options)
    {
        if (m_running)
            return false;

        struct winsize ws{
            .ws_row = options.termincalSize.rows,
            .ws_col = options.termincalSize.cols,
            .ws_xpixel = options.termincalSize.pixelWidth,
            .ws_ypixel = options.termincalSize.pixelHeight,
        };

        int masterFd = -1;

        pid_t pid = ::forkpty(&masterFd, nullptr, nullptr, &ws);
        if (pid < 0)
        {
            return false;
        }
        else if (pid == 0)
        {
            execChild(executable, args, options);
        }

        if (!m_pty.adopt(masterFd))
        {
            ::close(masterFd);
            ::kill(pid, SIGKILL);
            ::waitpid(pid, nullptr, 0);

            return false;
        }

        m_pid = pid;
        m_running = true;

        return true;
    }

    bool PtyProcess::startShell(std::string_view shell)
    {
        return spawn(shell);
    }

    void PtyProcess::terminate()
    {
        sendSignal(SIGTERM);
    }

    void PtyProcess::kill()
    {
        sendSignal(SIGKILL);
    }

    bool PtyProcess::sendSignal(int sig)
    {
        if (!m_running)
            return false;

        return ::kill(m_pid, sig);
    }

    bool PtyProcess::running() const
    {
        return m_running;
    }

    pid_t PtyProcess::pid() const
    {
        return m_pid;
    }

    std::optional<ProcessExitStatus> PtyProcess::tryWait()
    {
        if (!m_running)
            return std::nullopt;

        int status{0};
        pid_t ret = ::waitpid(m_pid, &status, WNOHANG);
        if (ret == 0)
            return std::nullopt;
        m_running = false;
        return ProcessExitStatus::fromWaitStatus(status);
    }

    ProcessExitStatus PtyProcess::wait()
    {
        int status{0};
        while (true)
        {
            pid_t ret = ::waitpid(m_pid, &status, 0);
            if (ret < 0)
                continue;
            break;
        }
        m_running = false;
        return ProcessExitStatus::fromWaitStatus(status);
    }

    bool PtyProcess::waitFor(std::chrono::milliseconds timeout)
    {
        auto start = std::chrono::steady_clock::now();
        while (true)
        {
            auto result = tryWait();
            if (result.has_value())
                return true;
            auto now = std::chrono::steady_clock::now();
            if (now - start >= timeout)
                return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ssize_t PtyProcess::read(std::span<std::byte> buffer)
    {
        return m_pty.read(buffer);
    }

    ssize_t PtyProcess::write(std::span<const std::byte> buffer)
    {
        return m_pty.write(buffer);
    }

    ssize_t PtyProcess::write(std::string_view text)
    {
        return m_pty.write(text);
    }

    bool PtyProcess::resize(const TerminalSize &size)
    {
        return m_pty.resize(size);
    }

    Pty &PtyProcess::pty()
    {
        return m_pty;
    }

    const Pty &PtyProcess::pty() const
    {
        return m_pty;
    }

    void PtyProcess::execChild(std::string_view executable, const std::vector<std::string> &args, const SpawnOptions &options)
    {
        if (!options.workingDirectory.empty())
            ::chdir(options.workingDirectory.c_str());
        if (options.clearEnvironment)
            ::clearenv();
        for (const auto &env : options.environment)
            ::putenv(const_cast<char *>(env.c_str()));

        std::vector<char *> argv;
        argv.push_back(const_cast<char *>(executable.data()));
        for (const auto &arg : args)
            argv.push_back(const_cast<char *>(arg.c_str()));
        argv.push_back(nullptr);

        ::execvp(executable.data(), argv.data());

        ::exit(127);
    }
}