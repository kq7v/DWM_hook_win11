#pragma once

#include <windows.h>
#include <atomic>
#include <mutex>

namespace sdk {
    class c_hooks {
        std::mutex mutex;
        HMODULE module = nullptr;
        bool installed = false;

        bool install( );

      public:

        std::atomic<bool> enabled { false };

        bool initialize( );
        void shutdown( );
    };

    extern c_hooks* const hooks;
}
