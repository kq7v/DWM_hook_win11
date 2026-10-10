#include <sdk/hooks/hooks.hpp>
#include <core/input/input.hpp>
#include <backends/dwm/dwm.hpp>
#include <features/menu/menu.hpp>

namespace sdk {
    namespace {
        c_hooks instance;
    }

    c_hooks* const hooks = &instance;

    bool c_hooks::initialize( ) {
        std::lock_guard lock( mutex );

        if( enabled ) {
            return true;
        }

        if( GetSystemMetrics( SM_CMONITORS ) != 1 || !install( ) ) {
            return false;
        }

        if( FAILED( core::input->initialize( module ) ) ) {
            return false;
        }

        backends::dwm->initialize( []( ) {
            features::menu->draw( );
        } );

        features::menu->opened = true;
        enabled = true;

        return true;
    }

    void c_hooks::shutdown( ) {
        std::lock_guard lock( mutex );

        enabled = false;
        core::input->shutdown( );
        backends::dwm->shutdown( );
    }
}
