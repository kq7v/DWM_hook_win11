#pragma once

#include <windows.h>
#include <array>

namespace core {
    struct mouse_event {
        int button;
        bool down;
        POINT position;
    };

    struct input_frame {
        POINT position { -1000, -1000 };
        float scale = 1, wheel = 0;
        std::array<mouse_event, 64> events { };
        std::size_t count = 0;
        bool reset = false;
    };
}
