#pragma once

#include <filesystem>
#include <vector>

#include "processExitStatus.hpp"
#include "pty.hpp"

namespace terminal
{
    class PtyProcess
    {
    public:
        struct SpawnOptions
        {
            TerminalSize termincalSize;
            std::filesystem::path workingDirectory;
            bool clearEnvironment;
            std::vector<std::string> environment;
        };

    public:
        PtyProcess() = default;
        ~PtyProcess();
        PtyProcess(const PtyProcess &) = delete;
        PtyProcess &operator=(const PtyProcess &) = delete;
        PtyProcess(PtyProcess &&) = default;
        PtyProcess &operator=(PtyProcess &&) = default;

    public:
        bool spawn(std::string_view executable, const std::vector<std::string> &args = {}, const SpawnOptions &options = {});
        bool startShell(std::string_view shell = "/bin/bash");

    public:
        void terminate();
        void kill();
        bool sendSignal(int sig);

    public:
        [[nodiscard]]
        bool running() const;
        [[nodiscard]]
        pid_t pid() const;

    public:
        std::optional<ProcessExitStatus> tryWait();
        ProcessExitStatus wait();
        bool waitFor(std::chrono::milliseconds timeout);

    public:
        ssize_t read(std::span<std::byte> buffer);
        ssize_t write(std::span<const std::byte> buffer);
        ssize_t write(std::string_view text);

    public:
        bool resize(const TerminalSize &size);

    public:
        [[nodiscard]]
        Pty &pty();
        [[nodiscard]]
        const Pty &pty() const;

    private:
        [[noreturn]]
        void execChild(std::string_view executable, const std::vector<std::string> &args, const SpawnOptions &options);

    private:
        Pty m_pty;
        pid_t m_pid{-1};
        bool m_running{false};
    };
}
