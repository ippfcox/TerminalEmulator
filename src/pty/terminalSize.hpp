#pragma once

#include <cstdint>

namespace terminal
{
    struct TerminalSize
    {
        uint16_t rows{24};
        uint16_t cols{80};

        uint16_t pixelWidth{0};
        uint16_t pixelHeight{0};
    };
}