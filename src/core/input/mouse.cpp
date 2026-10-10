#include <core/input/input.hpp>
#include <sdk/hooks/hooks.hpp>
#include <features/menu/menu.hpp>
#include <algorithm>

namespace core {
    bool c_input::process_mouse( WPARAM message, const MSLLHOOKSTRUCT& event ) {
        pending.position = event.pt;

        if( !sdk::hooks->enabled ) {
            return false;
        }

        bool swallow = false;
        const bool inside = features::menu->opened && PtInRect( &panel, event.pt );
        constexpr std::array down { WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN, WM_XBUTTONDOWN };
        constexpr std::array up { WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP, WM_XBUTTONUP };

        for( int i = 0; i < 4; ++i ) {
            const int button = i == 3 ? ( HIWORD( event.mouseData ) == XBUTTON1 ? 3 : 4 ) : i;
            const bool pressed = message == static_cast<WPARAM>( down[ i ] );
            const bool released = message == static_cast<WPARAM>( up[ i ] );

            if( !( pressed && inside ) && !( released && owned[ button ] ) ) {
                continue;
            }

            owned[ button ] = pressed;

            if( pending.count == pending.events.size( ) ) {
                pending.count = 0;
                pending.reset = true;
            }

            pending.events[ pending.count++ ] = { button, pressed, event.pt };
            swallow = true;
            break;
        }

        if( message == WM_MOUSEWHEEL && inside ) {
            pending.wheel += static_cast<short>( HIWORD( event.mouseData ) ) / 120.0f;
            swallow = true;
        }

        return swallow;
    }

    LRESULT CALLBACK c_input::mouse_hook( int code, WPARAM message, LPARAM data ) {
        if( code != HC_ACTION ) {
            return CallNextHookEx( nullptr, code, message, data );
        }

        bool swallow;
        {
            std::lock_guard lock( input->mutex );
            swallow = input->process_mouse( message, *reinterpret_cast<MSLLHOOKSTRUCT*>( data ) );
        }

        return swallow ? 1 : CallNextHookEx( nullptr, code, message, data );
    }

    void c_input::release( ) {
        std::lock_guard lock( mutex );
        panel = { };
        pending.count = 0;
        pending.wheel = 0;
        pending.reset = true;
    }

    void c_input::set_panel( RECT rect ) {
        std::lock_guard lock( mutex );

        panel = rect;
    }

    input_frame c_input::consume( ) {
        std::lock_guard lock( mutex );
        const auto frame = pending;
        pending.count = 0;
        pending.wheel = 0;
        pending.reset = false;

        return frame;
    }
}
