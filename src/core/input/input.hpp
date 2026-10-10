#pragma once

#include <core/input/state/frame.hpp>
#include <atomic>
#include <mutex>

namespace core {
    class c_input {
        std::mutex mutex;
        input_frame pending;
        RECT panel { };
        std::array<bool, 5> owned { };
        bool insert_down = false;

        HANDLE thread = nullptr, stop_event = nullptr, ready_event = nullptr;
        std::atomic<HRESULT> result { E_FAIL };

        static DWORD WINAPI run( void* module );
        DWORD poll( HMODULE module );
        void update_display( );
        void redraw( bool force = false );

        static LRESULT CALLBACK keyboard_hook( int code, WPARAM message, LPARAM data );
        static LRESULT CALLBACK mouse_hook( int code, WPARAM message, LPARAM data );
        bool process_mouse( WPARAM message, const MSLLHOOKSTRUCT& event, bool capture );

      public:

        HRESULT initialize( HMODULE module );
        void shutdown( );

        input_frame consume( );
        void set_panel( RECT rect );
        void release( );
    };

    extern c_input* const input;
}
