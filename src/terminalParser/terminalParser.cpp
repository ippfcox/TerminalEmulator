#include "terminalParser.hpp"

namespace terminal
{
    void TerminalParser::input(std::span<const std::byte> data)
    {
        for (std::byte b : data)
            processByte(static_cast<uint8_t>(b));
    }

    bool TerminalParser::pollEvent(TerminalEvent &event)
    {
        if (m_events.empty())
            return false;
        event = m_events.front();
        m_events.pop();
        return true;
    }

    void TerminalParser::processByte(uint8_t byte)
    {
        switch (m_state)
        {
        case State::Ground: processGround(byte); break;
        case State::Escape: processEscape(byte); break;
        case State::CSI: processCSI(byte); break;
        }
    }

    void TerminalParser::processGround(uint8_t byte)
    {
        switch (byte)
        {
        case 0x1B: m_state = State::Escape; return;
        case '\r': emit(CarriageReturnEvent{}); return;
        case '\n': emit(LineFeedEvent{}); return;
        case '\b': emit(BackspaceEvent{}); return;
        case '\t': emit(TabEvent{}); return;
        default: emit(PrintEvent{static_cast<char32_t>(byte)}); return;
        }
    }

    void TerminalParser::processEscape(uint8_t byte)
    {
        if (byte == '[')
        {
            m_csiBuffer.clear();
            m_state = State::CSI;
            return;
        }
        m_state = State::Ground;
    }

    std::vector<int> TerminalParser::parseCSIParams() const
    {
        std::vector<int> result;
        if (m_csiBuffer.empty())
        {
            result.push_back(0);
            return result;
        }

        int value = 0;
        bool hasDight = false;

        for (char c : m_csiBuffer)
        {
            if (c >= '0' && c <= '9')
            {
                value *= 10;
                value += c - '0';
                hasDight = true;
            }
            else if (c == ';')
            {
                if (hasDight)
                    result.push_back(value);
                else
                    result.push_back(0);
                value = 0;
                hasDight = false;
            }
        }

        if (hasDight)
            result.push_back(value);

        return result;
    }

    void TerminalParser::processCSI(uint8_t byte)
    {
        if ((byte > '0' && byte <= '9') || byte == ';')
        {
            m_csiBuffer.push_back(static_cast<char>(byte));
            return;
        }

        auto params = parseCSIParams();

        switch (byte)
        {
        case 'A': emit(CursorUpEvent{params[0] == 0 ? 1 : params[0]}); break;
        case 'B': emit(CursorDownEvent{params[0] == 0 ? 1 : params[0]}); break;
        case 'C': emit(CursorForwardEvent{params[0] == 0 ? 1 : params[0]}); break;
        case 'D': emit(CursorBackwardEvent{params[0] == 0 ? 1 : params[0]}); break;
        case 'H':
        case 'f':
        {
            int row = 1;
            int col = 1;
            if (params.size() >= 1)
                row = params[0];
            if (params.size() >= 2)
                col = params[1];
            emit(CursorPositionEvent{row, col});
        }
        break;
        case 'J': emit(ClearScreenEvent{}); break;
        case 'K': emit(ClearLineEvent{}); break;
        case 'm':
        {
            if (params.empty())
            {
                emit(ResetAttributesEvent{});
                break;
            }
            for (int p : params)
            {
                switch (p)
                {
                case 0: emit(ResetAttributesEvent{}); break;
                case 30: emit(SetForegroundColorEvent{Color::Black}); break;
                case 31: emit(SetForegroundColorEvent{Color::Red}); break;
                case 32: emit(SetForegroundColorEvent{Color::Green}); break;
                case 33: emit(SetForegroundColorEvent{Color::Yellow}); break;
                case 34: emit(SetForegroundColorEvent{Color::Blue}); break;
                case 35: emit(SetForegroundColorEvent{Color::Magenta}); break;
                case 36: emit(SetForegroundColorEvent{Color::Cyan}); break;
                case 37: emit(SetForegroundColorEvent{Color::White}); break;
                case 39: emit(SetForegroundColorEvent{Color::Default}); break;
                }
            }
        }
        break;
        }
        m_state = State::Ground;
    }
}