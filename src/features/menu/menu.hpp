#pragma once

#include <windows.h>
#include <atomic>

namespace features {
    class c_menu {
        unsigned clicks = 0;
        bool checked = false;
        float value = 0.5f;

      public:

        std::atomic<bool> opened { true };

        void draw( );
        void reset( );
    };

    extern c_menu* const menu;
}
