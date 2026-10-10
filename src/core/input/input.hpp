#pragma once

#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>

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

    class c_input {
        std::mutex mutex;
        input_frame pending;
        RECT panel { };
        std::array<bool, 5> owned { };
        bool insert_down = false;
        HANDLE thread = nullptr, stop_event = nullptr, ready_event = nullptr;
        std::atomic<HRESULT> result { E_FAIL };

        bool process_mouse( WPARAM message, const MSLLHOOKSTRUCT& event );
        static LRESULT CALLBACK mouse_hook( int code, WPARAM message, LPARAM data );
        void update_display( );
        void redraw( bool force = false );
        static LRESULT CALLBACK keyboard_hook( int code, WPARAM message, LPARAM data );
        static DWORD WINAPI run( void* module );
        DWORD poll( HMODULE module );

      public:

        HRESULT initialize( HMODULE module );
        void shutdown( );
        input_frame consume( );
        void set_panel( RECT rect );
        void release( );
    };

    extern c_input* const input;
}
