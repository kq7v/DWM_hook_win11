#include <sdk/dwm/dwm.hpp>
#include <sdk/dwm/detail/layout.hpp>

namespace sdk {
    namespace {
        c_dwm instance;
    }

    c_dwm* const dwm = &instance;

    bool c_dwm::initialize( ) {
        return detail::initialize_layout( GetModuleHandleW( L"dwmcore.dll" ) );
    }

    hook_targets c_dwm::targets( ) const noexcept {
        return detail::bindings.hooks;
    }

    void c_dwm::invalidate( void* manager ) const noexcept {
        if( manager && detail::bindings.force_dirty ) {
            detail::bindings.force_dirty( manager );
        }
    }

    void c_dwm::request_frame( ) const noexcept {
        if( detail::bindings.schedule ) {
            detail::bindings.schedule( 0, 0 );
        }
    }
}
