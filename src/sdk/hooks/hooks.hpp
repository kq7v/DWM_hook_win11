#pragma once

#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>

namespace sdk {
    class c_hooks {
        std::mutex mutex;
        std::array<void*, 4> addresses { };
        bool installed = false;
        bool owns_minhook = false;

        bool create( );
        void rollback( );
        HRESULT initialize_input( );

      public:

        std::atomic<bool> enabled { false };

        HRESULT initialize( );
        HRESULT shutdown( );
    };

    extern c_hooks* const hooks;
}
