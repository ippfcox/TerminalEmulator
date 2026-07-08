#pragma once

#include "terminalEvent.hpp"
#include <span>
#include <vector>
#include <string>
#include <queue>

namespace terminal
{
    class TerminalParser
    {
    public:
        TerminalParser() = default;

    public:
        void input(std::span<const std::byte> data);

    public:
        bool pollEvent(TerminalEvent &event);

    private:
        enum class State
        {
            Ground,
            Escape,
            CSI,
        };

    private:
        void processByte(uint8_t byte);
        void processGround(uint8_t byte);
        void processEscape(uint8_t byte);
        void processCSI(uint8_t byte);

    private:
        void emit(const TerminalEvent &event);

    private:
        std::vector<int> parseCSIParams() const;

    private:
        State m_state{State::Ground};
        std::string m_csiBuffer;
        std::queue<TerminalEvent> m_events;
    };
}