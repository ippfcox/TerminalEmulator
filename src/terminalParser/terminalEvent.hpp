#pragma once

#include <variant>

namespace terminal
{
    enum class Color
    {
        Default,
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };

    struct PrintEvent
    {
        char32_t codepoint;
    };

    struct CarriageReturnEvent
    {
    };

    struct LineFeedEvent
    {
    };

    struct BackspaceEvent
    {
    };

    struct TabEvent
    {
    };

    struct CursorUpEvent
    {
        int count;
    };

    struct CursorDownEvent
    {
        int count;
    };

    struct CursorForwardEvent
    {
        int count;
    };

    struct CursorBackwardEvent
    {
        int count;
    };

    struct CursorPositionEvent
    {
        int row;
        int col;
    };

    struct ClearScreenEvent
    {
        int mode;
    };

    struct ClearLineEvent
    {
        int mode;
    };

    struct SetForegroundColorEvent
    {
        Color color;
    };

    struct ResetAttributesEvent
    {
    };

    using TerminalEvent = std::variant<
        PrintEvent,
        CarriageReturnEvent,
        LineFeedEvent,
        BackspaceEvent,
        TabEvent,
        CursorUpEvent,
        CursorDownEvent,
        CursorForwardEvent,
        CursorBackwardEvent,
        CursorPositionEvent,
        ClearScreenEvent,
        ClearLineEvent,
        SetForegroundColorEvent,
        ResetAttributesEvent>;
}