#include <core/input/input.hpp>
#include <sdk/hooks/hooks.hpp>
#include <features/menu/menu.hpp>

namespace core {
    namespace {
        DWORD WINAPI stop_hooks( void* ) {
            sdk::hooks->shutdown( );
            return 0;
        }
    }

    LRESULT CALLBACK c_input::keyboard_hook( int code, WPARAM message, LPARAM data ) {
        if( code != HC_ACTION || !sdk::hooks->enabled ) {
            return CallNextHookEx( nullptr, code, message, data );
        }

        const auto& event = *reinterpret_cast<const KBDLLHOOKSTRUCT*>( data );
        const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;

        if( event.vkCode == VK_INSERT ) {
            if( down && !input->insert_down ) {
                features::menu->opened = !features::menu->opened;
                input->release( );
            }

            input->insert_down = down;
            return 1;
        }

        if( down && event.vkCode == VK_END ) {
            sdk::hooks->enabled = false;

            if( const auto controller = CreateThread( nullptr, 0, stop_hooks, nullptr, 0, nullptr ) ) {
                CloseHandle( controller );
            }

            SetEvent( input->stop_event );
            return 1;
        }

        return CallNextHookEx( nullptr, code, message, data );
    }
}
