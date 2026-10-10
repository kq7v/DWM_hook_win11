#include <core/input/input.hpp>
#include <sdk/hooks/hooks.hpp>
#include <features/menu/menu.hpp>

namespace core {
    LRESULT CALLBACK c_input::mouse_hook( int code, WPARAM message, LPARAM data ) {
        if( code != HC_ACTION || !sdk::hooks->enabled ) {
            return CallNextHookEx( nullptr, code, message, data );
        }

        const auto& event = *reinterpret_cast<const MSLLHOOKSTRUCT*>( data );
        const bool swallowed = input->process_mouse( message, event, features::menu->opened );

        return swallowed ? 1 : CallNextHookEx( nullptr, code, message, data );
    }
}
